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

using namespace std;

const int DEFAULT_STACK_SZIE = 8 * 1024;
const int MAX_COROUTINE_SIZE = 2;
const int CO_THREAD_SIZE = 2;

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
	CoSuspendState	_suspend_state;

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

	void switch_to_main();

private:
	void process();

public:
	vector<shared_ptr<Coroutine>> _lst_free;
	vector<shared_ptr<Coroutine>> _lst_ready;

	ucontext_t _main_ctx;

	int	 _running_id;
};

class CoManager
{
public:
	CoManager();
	~CoManager();

	bool create(const function<void()>& func);

	shared_ptr<Coroutine> get_co(int id);

	shared_ptr<Coroutine> get_running_co();

	shared_ptr<Coroutine> get_ready_co();

	void add_free_co(const shared_ptr<Coroutine>& co);

	void add_free_co(const vector<shared_ptr<Coroutine>>& cos);

	void add_ready_co(const shared_ptr<Coroutine>& co);

	void add_suspend_co(const shared_ptr<Coroutine>& co);

	bool remove_suspend(int id);

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

	vector<thread> _threads;

	mutex _mutex;
};

extern CoManager g_manager;

extern thread_local shared_ptr<CoSchedule> g_schedule;

inline int getcid()
{
	return g_schedule->_running_id;
}

inline int gettid()
{
	return syscall(SYS_gettid);
}

#endif
