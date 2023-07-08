#ifndef __CO_SEMAPHORE_H__
#define __CO_SEMAPHORE_H__

#include <memory>
#include <list>
#include "common/semaphore.h"

using namespace std;

#define __SEM_DEBUG

class Coroutine;

class CoSemaphore
{
public:
    CoSemaphore(int value = 0) {
        _sem = new Semaphore();
        _value.store(value);

#ifdef __SEM_DEBUG
        _send_count.store(0);

        _send_busy_try.store(0);
        _send_yield_try.store(0);

        _recv_busy_try.store(0);
        _recv_yield_try.store(0);
#endif
    }

    ~CoSemaphore() {
        delete _sem;
#ifdef __SEM_DEBUG
        printf("count:%d\n", _send_count.load());
        printf("send|busy:%d, yield:%d\n", _send_busy_try.load(), _send_yield_try.load());
        printf("recv|busy:%d, yield:%d\n", _recv_busy_try.load(), _recv_yield_try.load());
#endif
    }

    int get_value();

    void signal();

    void wait();

private:
    // 信号量值（31位）|锁标识（1位）
    atomic<int> _value;

    list<shared_ptr<Coroutine>> _lst_wait;

    Semaphore* _sem;

#ifdef __SEM_DEBUG
    atomic<int> _send_count;

    atomic<int> _send_busy_try;
    atomic<int> _send_yield_try;

    atomic<int> _recv_busy_try;
    atomic<int> _recv_yield_try;
#endif

};

#endif