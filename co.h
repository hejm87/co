#ifndef __CO_H__
#define __CO_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ucontext.h>

#include <memory>
#include <atomic>
#include <functional>

#include <mutex>
#include <thread>

#include <map>
#include <list>
#include <vector>

#include <sys/syscall.h>

#include "common/any.h"

using namespace std;

const int DEFAULT_STACK_SZIE = 100 * 1024;
const int MAX_COROUTINE_SIZE = 1000;
const int CO_THREAD_SIZE = 2;
const int TIMER_THREAD_SIZE = 2;

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

	void resume(shared_ptr<Coroutine> co);

	void switch_to_main(const function<void()>& do_after_switch_func = nullptr);

private:
	void process();

public:
	vector<shared_ptr<Coroutine>> _lst_free;
	vector<shared_ptr<Coroutine>> _lst_ready;
	vector<shared_ptr<Coroutine>> _lst_resume;	// 将协程放回g_manager，避免调度线程竞�?

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

	void sleep(unsigned int sec);

	void sleep_ms(unsigned int msec);

	shared_ptr<Coroutine> get_co(int id);

	shared_ptr<Coroutine> get_free_co();

	shared_ptr<Coroutine> get_running_co();

	shared_ptr<Coroutine> get_ready_co();

	void add_free_co(const shared_ptr<Coroutine>& co);

	void add_free_co(const vector<shared_ptr<Coroutine>>& cos);

	void add_ready_co(const shared_ptr<Coroutine>& co, bool priority = false);

	void add_suspend_co(const shared_ptr<Coroutine>& co);

	bool resume_co(int id);

	void lock();

	void unlock();

	mutex* get_locker();

	static void co_run(int id);

public:
	atomic<bool> _is_stop;

private:
	vector<shared_ptr<Coroutine>>	_lst_co;
	list<shared_ptr<Coroutine>>		_lst_free;
	list<shared_ptr<Coroutine>>		_lst_ready;

	map<int, shared_ptr<Coroutine>> _map_suspend;

	multimap<long, int> _lst_sleep;

	vector<thread> _threads;

	mutex _mutex;
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

inline int gettid()
{
	return syscall(SYS_gettid);
}

// �?否在协程线程�?�?
inline bool is_in_co_env()
{
	return g_schedule ? true : false;
}

#endif
