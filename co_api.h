#ifndef __CO_API_H__
#define __CO_API_H__

#include "co.h"
#include "co_timer.h"
#include "common/common.h"

using namespace std;

class CoApi
{
public:
    template <class F, class... Args>
    static auto create(F&& f, Args&&... args)
        -> CoAwait<typename std::enable_if<!std::is_void<typename std::result_of<F(Args...)>::type>::value, typename std::result_of<F(Args...)>::type>::type>
    {
        using utype = typename std::result_of<F(Args...)>::type;
        auto co_wait = CoAwait<utype>();
        auto co_func = bind(forward<F>(f), forward<Args>(args)...); 
        g_manager.create([co_func, co_wait] {
            *co_wait._result = co_func();
            co_wait._chan->close();
        });
        return co_wait;
    }

    template <class F, class... Args>
    static auto create(F&& f, Args&&... args)
        -> CoAwait<typename std::enable_if<std::is_void<typename std::result_of<F(Args...)>::type>::value>::type>
    {
        auto co_wait = CoAwait<void>();
        auto co_func = bind(forward<F>(f), forward<Args>(args)...); 
        g_manager.create([co_func, co_wait] {
            co_func();
            co_wait._chan->close();
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

    static CoTimerId set_timer(size_t delay_ms, const std::function<void()>& func) {
        return g_manager.set_timer(delay_ms, func);
    }

	static bool cancel_timer(CoTimerId& id) {
        return g_manager.cancel_timer(id);
    }

	static CoTimerState get_timer_state(const CoTimerId& id) {
        return g_manager.get_timer_state(id);
    }

    static int getcid() {
        getcid();
    }
};

#endif