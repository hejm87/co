#ifndef __CO_API_H__
#define __CO_API_H__

#include "co.h"
#include "common/common.h"

using namespace std;
/*
    template <class F, class... Args>
    static auto create_with_promise(F&& f, Args&&... args)
        -> CoAwaiter<typename std::enable_if<std::is_void<typename std::result_of<F(Args...)>::type>::value>::type>
    {
        CoAwaiter<void> awaiter;
        auto co_func = std::bind(forward<F>(f), forward<Args>(args)...);
        auto func = [co_func, &awaiter]() {
            co_func();
            awaiter.set_value();
        };
        Singleton<CoSchedule>::get_instance()->create(false, func);
        return awaiter;
    }
*/
class CoApi
{
public:
    template <class F, class... Args>
    static auto create(F&& f, Args&&... args)
        -> CoAwait<typename std::enable_if<!std::is_void<typename std::result_of<F(Args...)>::type>::value, typename std::result_of<F(Args...)>::type>::type>
    {
        printf("[%s] apiaaaaaaaaaaaa, create co with type\n", date_ms().c_str());
        using utype = typename std::result_of<F(Args...)>::type;
        auto co_wait = CoAwait<utype>();
        auto co_func = bind(forward<F>(f), forward<Args>(args)...); 
        g_manager.create([co_func, co_wait] {
            *co_wait._result = co_func();
            co_wait._chan->close();
            printf("[%s] apiaaaaaaaaaaaa, after signal\n", date_ms().c_str());
        });
        return co_wait;
    }

    template <class F, class... Args>
    static auto create(F&& f, Args&&... args)
        -> CoAwait<typename std::enable_if<std::is_void<typename std::result_of<F(Args...)>::type>::value>::type>
    {
        printf("[%s] apiaaaaaaaaaaaa, create co no type\n", date_ms().c_str());
        auto co_wait = CoAwait<void>();
        auto co_func = bind(forward<F>(f), forward<Args>(args)...); 
        g_manager.create([co_func, co_wait] {
            co_func();
            co_wait._chan->close();
            printf("[%s] apiaaaaaaaaaaaa, after signal\n", date_ms().c_str());
        });
        return co_wait;
    }

    static void yield() {
        g_manager.yield();
    }

    static void sleep(int sec) {
        g_manager.sleep(sec);
    }

    static void sleep_ms(int msec) {
        g_manager.sleep_ms(msec);
    }

    static int getcid() {
        getcid();
    }
};

#endif