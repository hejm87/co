#ifndef __CO_H__
#define __CO_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ucontext.h>

#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>

#include <vector>
#include <map>

#include <sys/syscall.h>

using namespace std;

const int DEFAULT_STACK_SZIE = 8 * 1024;
const int MAX_COROUTINE_SIZE = 2;
const int CO_THREAD_SIZE = 2;

enum CoState{
	FREE,
	RUNNABLE,
	RUNNING,
	SUSPEND
};

class Coroutine
{
public:
	Coroutine(int id);
	~Coroutine();

public:
	int		_id;
	bool	_init;
	CoState	_state;	
	char*	_stack;

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

	bool create(function<void()> func);

	void suspend();

	shared_ptr<Coroutine> get_co(int id);

	shared_ptr<Coroutine> get_ready_co();

	void add_free_co(const shared_ptr<Coroutine>& co);

	void add_free_co(const vector<shared_ptr<Coroutine>>& cos);

	void add_ready_co(const shared_ptr<Coroutine>& co);

	void add_suspend_co(const shared_ptr<Coroutine>& co);

	static void co_run(int id);

public:
	atomic<bool> _is_stop;

private:
	vector<shared_ptr<Coroutine>> _lst_co;
	vector<shared_ptr<Coroutine>> _lst_free;
	vector<shared_ptr<Coroutine>> _lst_ready;

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
