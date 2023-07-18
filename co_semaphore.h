#ifndef __CO_SEMAPHORE_H__
#define __CO_SEMAPHORE_H__

#include <memory>
#include <list>
#include "co_exception.h"
#include "utils.h"
//#include "common/semaphore.h"


using namespace std;

#define __SEM_DEBUG

const int _MAX_SEM_POSITIVE = 1073741823;
const int _MAX_SEM_NEGATIVE = -1073741824;

class Coroutine;

class CoSemaphore
{
public:
    CoSemaphore(int value = 0) {
        if (value > _MAX_SEM_POSITIVE || value < _MAX_SEM_NEGATIVE) {
            THROW_EXCEPTION(
                CO_ERROR_PARAM_INVALID, 
                "CoSemaphore init value:%d out of range", 
                value
            );
        }
        _value.store(value << 1);

#ifdef __SEM_DEBUG
        _send_count.store(0);

        _send_busy_try.store(0);
        _send_yield_try.store(0);

        _recv_busy_try.store(0);
        _recv_yield_try.store(0);
#endif
    }

	CoSemaphore(const CoSemaphore&) = delete;

    ~CoSemaphore() {
       // delete _sem;
#ifdef __SEM_DEBUG
        printf("count:%d\n", _send_count.load());
        printf("send|busy:%d, yield:%d\n", _send_busy_try.load(), _send_yield_try.load());
        printf("recv|busy:%d, yield:%d\n", _recv_busy_try.load(), _recv_yield_try.load());
#endif
    }

	CoSemaphore& operator=(const CoSemaphore&) = delete;

    int get_value();

    bool signal();

    void wait();

private:
    // 信号量值（31位）|锁标识（1位）
    atomic<int> _value;

    list<shared_ptr<Coroutine>> _lst_wait;

#ifdef __SEM_DEBUG
    atomic<int> _send_count;

    atomic<int> _send_busy_try;
    atomic<int> _send_yield_try;

    atomic<int> _recv_busy_try;
    atomic<int> _recv_yield_try;
#endif

};

#endif
