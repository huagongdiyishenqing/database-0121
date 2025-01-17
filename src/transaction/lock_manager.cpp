#include "transaction/lock_manager.h"

#include <iostream>

#include "common/exceptions.h"

namespace huadb {

    bool LockManager::LockTable(xid_t xid, LockType lock_type, oid_t oid) {
        // 对数据表加锁，成功加锁返回 true，如果数据表已被其他事务加锁，且锁的类型不相容，返回 false
        // 如果本事务已经持有该数据表的锁，根据需要升级锁的类型
        // LAB 3 BEGIN
        Lock lock{
                lock_type,
                LockGranularity::TABLE,
                xid,
                {0, 0},
        };

        // 遍历该数据表下的所有表锁
        // 不相容
        for (auto &iter: locks_[oid]) {
            if (iter.xid_ != xid && !Compatible(iter.lock_type_, lock_type) && iter.granularity_ == LockGranularity::TABLE) {
                return false;
            }
        }
        // 升级
        for (auto &iter: locks_[oid]) {
            if (iter.xid_ == xid && iter.granularity_ == LockGranularity::TABLE) {
                iter.lock_type_ = Upgrade(iter.lock_type_, lock_type);
                return true;
            }
        }
        // 加锁
        locks_[oid].emplace_back(lock);

        return true;
    }

    bool LockManager::LockRow(xid_t xid, LockType lock_type, oid_t oid, Rid rid) {
        // 对数据行加锁，成功加锁返回 true，如果数据行已被其他事务加锁，且锁的类型不相容，返回 false
        // 如果本事务已经持有该数据行的锁，根据需要升级锁的类型
        // LAB 3 BEGIN
        Lock lock{
                lock_type,
                LockGranularity::ROW,
                xid,
                rid,
        };

        // 遍历该数据表下的所有行锁
        // 不相容
        for (auto &iter: locks_[oid]) {
            if (iter.rid_.slot_id_ == rid.slot_id_ && iter.rid_.page_id_ == rid.page_id_ &&
                iter.granularity_ == LockGranularity::ROW &&
                iter.xid_ != xid &&
                !Compatible(iter.lock_type_, lock_type)) {
                return false;
            }
        }
        // 升级
        for (auto &iter: locks_[oid]) {
            if (iter.rid_.slot_id_ == rid.slot_id_ && iter.rid_.page_id_ == rid.page_id_ &&
                iter.granularity_ == LockGranularity::ROW &&
                iter.xid_ == xid) {
                iter.lock_type_ = Upgrade(iter.lock_type_, lock_type);
                return true;
            }
        }
        // 加锁
        locks_[oid].emplace_back(lock);
        return true;
    }

    void LockManager::ReleaseLocks(xid_t xid) {
        // 释放事务 xid 持有的所有锁
        // LAB 3 BEGIN
        for (auto [oid, lock_vec] : locks_) {
            for (auto iter = lock_vec.begin(); iter != lock_vec.end();) {
                auto lock = *iter;
                if (lock.xid_ == xid) {
                    iter = lock_vec.erase(iter);
                } else {
                    iter++;
                }
            }
            locks_[oid] = lock_vec;
        }
    }

    void LockManager::SetDeadLockType(DeadlockType deadlock_type) { deadlock_type_ = deadlock_type; }

    bool LockManager::Compatible(LockType type_a, LockType type_b) {
        // 判断锁是否相容
        // LAB 3 BEGIN
        return compatible_matrix_[static_cast<int>(type_a)][static_cast<int>(type_b)];
    }

    LockType LockManager::Upgrade(LockType self, LockType other) {
        // 升级锁类型
        // LAB 3 BEGIN
        return upgrade_matrix_[static_cast<int>(self)][static_cast<int>(other)];
    }

}  // namespace huadb
