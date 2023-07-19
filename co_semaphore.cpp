#include <sched.h>
#include <assert.h>
#include "co.h"
#include "co_semaphore.h"
#include "utils.h"
#include "common/semaphore.h"

#define IS_LOCK(x)    ((x) & 0x1 ? true : false)

#define SET_LOCK(x)   ((x) | 0x1)
#define SET_UNLOCK(x) ((x) & 0xfffffffe)

#define GET_VALUE(x) ((int)((x) >> 1))
#define INC_VALUE(x) ((x) + 2)
#define DEC_VALUE(x) ((x) - 2)

const int TRY_LOCK_COUNT = 10;

int CoSemaphore::get_value()
{
    return GET_VALUE(_value.load());
}

bool CoSemaphore::signal()
{
    int try_count = 0;
    do {
        auto value = _value.load();
        if (GET_VALUE(value) > _MAX_SEM_POSITIVE) {
            return false;
        }
        auto is_lock = IS_LOCK(value);
        try_count++;
        if (is_lock) {
            if (try_count <= TRY_LOCK_COUNT) {
#ifdef __SEM_DEBUG
                _send_busy_try++;
#endif
                continue ;
            }
        } else {
            auto new_value = INC_VALUE(SET_LOCK(value));
            // 上锁顺带信号值加一
#ifdef __SEM_DEBUG
            _send_count++;
#endif
            if (_value.compare_exchange_strong(value, new_value)) {
                auto unlock_value = SET_UNLOCK(new_value);
               // printf(
               //     "[%s] sssssssssssssss, tid:%d, cid:%d, signal, lock & inc_value, value:%u\n", 
               //     date_ms().c_str(),
               //     gettid(),
               //     getcid(),
               //     GET_VALUE(_value.load())
               // );
                if (_lst_wait.size()) {
                    auto co = _lst_wait.front();
                    _lst_wait.pop_front();
                   // printf(
                   //     "[%s] sssssssssssssss, tid:%d, cid:%d, signal, resume wait_cid:%d\n", 
                   //     date_ms().c_str(),
                   //     gettid(),
                   //     getcid(),
                   //     co->_id
                   // );
                    _value.store(unlock_value);
                    auto ret = g_manager.resume_co(co->_id);
                    assert(ret);
                } else {
                    _value.store(unlock_value);
                }
                // 解锁
               // printf(
               //     "[%s] sssssssssssssss, tid:%d, cid:%d, signal, unlock\n", 
               //     date_ms().c_str(),
               //     gettid(),
               //     getcid()
               // );
                break ;
            }
        }
        // 超过锁尝试次数，则休眠一会等待机会（区分协程环境 or 线程环境）
        if (try_count > TRY_LOCK_COUNT) {
#ifdef __SEM_DEBUG
            _send_yield_try++;
#endif
            if (is_in_co_env()) {
                g_manager.yield();
            } else {
                sched_yield();
            }
            try_count = 0;
        }
    } while (1);
    return true;
}

void CoSemaphore::wait()
{
    int try_count = 0;
    do {
        auto value = _value.load();
        auto is_lock = IS_LOCK(value);
        try_count++;
        if (is_lock) {
            if (try_count <= TRY_LOCK_COUNT || GET_VALUE(value) < _MAX_SEM_NEGATIVE) {
#ifdef __SEM_DEBUG
                _recv_busy_try++;
#endif
                continue ;
            }
        } else {
            auto new_value = DEC_VALUE(SET_LOCK(value));
            if (_value.compare_exchange_strong(value, new_value)) {
               // printf(
               //     "[%s] sssssssssssssss, tid:%d, cid:%d, wait, lock & dec_value, value:%d\n", 
               //     date_ms().c_str(),
               //     gettid(),
               //     getcid(),
               //     GET_VALUE(_value.load())
               // );
                auto unlock_value = SET_UNLOCK(new_value);
                auto wait_count = GET_VALUE(new_value);
                if (wait_count >= 0) {
                   // printf(
                   //     "[%s] sssssssssssssss, tid:%d, cid:%d, wait, get sem\n", 
                   //     date_ms().c_str(),
                   //     gettid(),
                   //     getcid()
                   // );
                    _value.store(unlock_value);
                    return ;
                }
                Semaphore sem;
                shared_ptr<Coroutine> co;
                if (is_in_co_env()) {
                    co = g_manager.get_running_co();
                } else {
                    co = g_manager.get_free_co();
                    co->_func = [this, &sem] {
                        sem.signal();
                    };
                }
                _lst_wait.push_back(co);
               // _value.store(unlock_value);

                co->_state = SUSPEND;
		        co->_suspend_state = SUSPEND_LOCK;
                if (is_in_co_env()) {
                   // printf(
                   //     "[%s] sssssssssssssss, tid:%d, cid:%d, suspend to wait sem\n", 
                   //     date_ms().c_str(), 
                   //     gettid(),
                   //     getcid()
                   // );
                   // g_schedule->switch_to_main([this, unlock_value] {
                   //     _value.store(unlock_value);
                   // });
                    g_manager.switch_to_main([this, unlock_value] {
                        _value.store(unlock_value);
                    });
                } else {
                   // printf(
                   //     "[%s] sssssssssssssss, tid:%d, cid:%d, suspend to wait sem\n", 
                   //     date_ms().c_str(), 
                   //     gettid(),
                   //     getcid()
                   // );
                    g_manager.add_suspend_co(co);
                    _value.store(unlock_value);
                    sem.wait();
                }
                break ;
            }
        }
        // 超过锁尝试次数，则休眠一会等待机会（区分协程环境 or 线程环境）
        if (try_count > TRY_LOCK_COUNT) {
#ifdef __SEM_DEBUG
            _recv_yield_try++;
#endif
            if (is_in_co_env()) {
                g_manager.yield();
            } else {
                sched_yield();
            }
            try_count = 0;
        }
    } while (1);
}