#ifndef __CO_H__
#define __CO_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ucontext.h>

#include <memory>
#include <atomic>
#include <functional>
#include <utility>

#include <mutex>
#include <thread>
#include <condition_variable>

#include <map>
#include <list>
#include <vector>

#include "common/any.h"
#include "co_channel.h"
#include "co_timer.h"

using namespace std;

class CoApi;

const int DEFAULT_STACK_SZIE = 100 * 1024;
const int MAX_COROUTINE_COUNT = 1000;
const int CO_THREAD_COUNT = 2;
const int CO_TIMER_THREAD_COUNT = 2;

enum CoState
{
	FREE,
	RUNNABLE,
	RUNNING,
	SUSPEND
};

enum CoSuspendState
{
	SUSPEND_NONE = 0,
	SUSPEND_LOCK,
	SUSPEND_SLEEP,
	SUSPEND_CHANNEL,
	SUSPEND_IO_BLOCK,
};

enum CoChannelBlockType
{
	CHANNEL_BLOCK_NULL = 0,
	CHANNEL_BLOCK_SEND,
	CHANNEL_BLOCK_RECV,
};

struct BlockChannelParam
{
	CoChannelBlockType type;	
	Any param;
};

class Coroutine
{
public:
	Coroutine(int id);
	~Coroutine();

public:
	int		_id;
	bool	_init;
	char*	_stack;

	CoState	_state;
	CoSuspendState _suspend_state;

	BlockChannelParam _channel_param;

	ucontext_t _ctx;
	function<void()> _func;
};

class CoSchedule : public std::enable_shared_from_this<CoSchedule>
{
public:
	CoSchedule();
	~CoSchedule();

	void run();

	void yield();

	void switch_to_main(const function<void()>& do_after_switch_func = nullptr);

private:
	void process();

public:
	ucontext_t _main_ctx;

	int	 _running_id;

	function<void()> _do_after_switch_func;
};

class CoManager
{
public:
	CoManager();
	~CoManager();

	bool create(const function<void()>& func);

	shared_ptr<Coroutine> create_with_co(const function<void()>& func);

	void switch_to_main(const function<void()>& do_after_switch_func = nullptr);

	void yield();

	void sleep(unsigned int sec);

	void sleep_ms(unsigned int msec);

	CoTimerId set_timer(size_t delay_ms, const std::function<void()>& func);

	int cancel_timer(CoTimerId& id);

	CoTimerState get_timer_state(const CoTimerId& id);

	shared_ptr<Coroutine> get_co(int id);

	shared_ptr<Coroutine> get_free_co();

	shared_ptr<Coroutine> get_running_co();

	shared_ptr<Coroutine> get_ready_co();

	void add_free_co(const shared_ptr<Coroutine>& co);

	void add_ready_co(const shared_ptr<Coroutine>& co, bool priority = false);

	void add_suspend_co(const shared_ptr<Coroutine>& co);

	bool resume_co(int id);

	void lock();

	void unlock();

	static void co_run(int id);

public:
	atomic<bool> _is_stop;

private:
	vector<shared_ptr<Coroutine>>	_lst_co;
	list<shared_ptr<Coroutine>>		_lst_free;
	list<shared_ptr<Coroutine>>		_lst_ready;

	map<int, shared_ptr<Coroutine>> _map_suspend;

	CoTimer* _timer;

	vector<thread> _threads;

	mutex _mutex;
	condition_variable _cv;
};

template <class T>
class CoAwait
{
friend class CoApi;
public:
	CoAwait(const CoAwait& obj) {
		_chan = obj._chan;
		_result = obj._result;
	}

	T wait() {
		if (_chan->is_close()) {
			THROW_EXCEPTION(CO_ERROR_INNER_EXCEPTION, "awaiter error");
		}
		try {
			T obj;
			*_chan >> obj;
		} catch (CoException& ex) {
			if (ex.code() != CO_ERROR_CHANNEL_CLOSE) {
				throw ex;
			}
		}
		return *_result;
	}

private:
	CoAwait& operator=(const CoAwait& obj) = delete;

	CoAwait() {
		_chan = shared_ptr<CoChannel<T>>(new CoChannel<T>());
		_result = shared_ptr<T>(new T);
	}

	shared_ptr<CoChannel<T>> _chan;
	shared_ptr<T>  _result;
};

template <>
class CoAwait<void>
{
friend class CoApi;
public:
	CoAwait(const CoAwait& obj) {
		_chan = obj._chan;
	}

	void wait() {
		if (_chan->is_close()) {
			THROW_EXCEPTION(CO_ERROR_INNER_EXCEPTION, "awaiter error");
		}
		try {
			int v;
			*_chan >> v;
		} catch (CoException& ex) {
			if (ex.code() != CO_ERROR_CHANNEL_CLOSE) {
				throw ex;
			}
		}
	}

private:
	CoAwait& operator=(const CoAwait& obj) = delete;

	CoAwait() {
		_chan = shared_ptr<CoChannel<int>>(new CoChannel<int>);
	}

	shared_ptr<CoChannel<int>> _chan;
};

extern CoManager g_manager;

extern thread_local shared_ptr<CoSchedule> g_schedule;

inline int getcid()
{
	int cid = -1;
	if (g_schedule) {
		cid = g_schedule->_running_id;
	}
	return cid;
}

// �??否在协程线程�??�??
inline bool is_in_co_env()
{
	return g_schedule ? true : false;
}

#endif
