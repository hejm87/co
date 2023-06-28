#ifndef __CO_H__
#define __CO_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ucontext.h>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <sys/syscall.h>

using namespace std;

const int DEFAULT_STACK_SZIE = 8 * 1024;
const int MAX_COROUTINE_SIZE = 100;
const int CO_THREAD_SIZE = 3;

enum CoState{
	FREE,
	RUNNABLE,
	RUNNING,
	SUSPEND
};

class Coroutine
{
public:
	Coroutine(int id) {
		_id = id;
		_state = FREE;
		_stack = (char*)malloc(DEFAULT_STACK_SZIE);
	}

	~Coroutine() {
		free(_stack);
	}

public:
	int		_id;
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

	shared_ptr<Coroutine> get_co(int id);

	shared_ptr<Coroutine> get_ready_co();

	void add_free_co(const vector<shared_ptr<Coroutine>>& cos);

	static void co_run(int id);

public:
	atomic<bool> _is_stop;

private:
	vector<shared_ptr<Coroutine>> _lst_co;
	vector<shared_ptr<Coroutine>> _lst_free;
	vector<shared_ptr<Coroutine>> _lst_ready;

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
