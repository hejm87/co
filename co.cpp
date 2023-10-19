#include <unistd.h>
#include <assert.h>
#include <sched.h>
#include "co.h"
#include "co_exception.h"
#include "utils.h"
#include "common/common_utils.h"

CoManager g_manager;

thread_local shared_ptr<CoSchedule> g_schedule;

Coroutine::Coroutine(int id)
{
    _id = id;
	_init  = false;
    _state = FREE;
    _stack = (char*)malloc(DEFAULT_STACK_SZIE);
}

Coroutine::~Coroutine()
{
    free(_stack);
}

CoSchedule::CoSchedule()
{
	_running_id = -1;
}

CoSchedule::~CoSchedule()
{
	;
}

void CoSchedule::run()
{
	g_schedule = shared_from_this();

	getcontext(&_main_ctx);

	while (!g_manager._is_stop.load()) {

		auto co = g_manager.get_ready_co();
		if (!co) {
		//	assert(g_manager._is_stop);
		//	break ;
			continue ;
		}

		_running_id = co->_id;

		if (!co->_init) {

			co->_init = true;

			getcontext(&(co->_ctx));

			co->_ctx.uc_stack.ss_sp = co->_stack;
			co->_ctx.uc_stack.ss_size  = DEFAULT_STACK_SZIE;
			co->_ctx.uc_stack.ss_flags = 0;
			co->_ctx.uc_link = NULL;

    		makecontext(&(co->_ctx), (void (*)(void))(CoManager::co_run), 1, co->_id);
		}

		_running_id = co->_id;

		swapcontext(&_main_ctx, &(co->_ctx));

		_running_id = -1;

		if (co->_state == FREE) {
		//	printf("[%s] ++++++++++ co_schedule, co_exit, tid:%d, cid:%d\n", date_ms().c_str(), gettid(), co->_id);
			g_manager.add_free_co(co);
		} else if (co->_state == RUNNABLE) {
		//	printf("[%s] ++++++++++ co_schedule, co_yield, tid:%d, cid:%d\n", date_ms().c_str(), gettid(), co->_id);
			g_manager.add_ready_co(co);
		} else if (co->_state == SUSPEND) {
		//	printf("[%s] ++++++++++ co_schedule, co_suspend, tid:%d, cid:%d\n", date_ms().c_str(), gettid(), co->_id);
			g_manager.add_suspend_co(co);
		}

		if (_do_after_switch_func) {
			_do_after_switch_func();
			_do_after_switch_func = nullptr;
		}
	}
	printf("co_schedule, tid:%d, thread end\n", gettid());
}

void CoSchedule::yield()
{
	auto co = g_manager.get_co(_running_id);
	co->_state = RUNNABLE;

	switch_to_main();
}

void CoSchedule::switch_to_main(const function<void()>& do_after_switch_func)
{
	assert(_running_id != -1);
	_do_after_switch_func = do_after_switch_func;

	auto co = g_manager.get_co(_running_id);
	swapcontext(&(co->_ctx), &_main_ctx);
}

CoManager::CoManager()
{
	for (int i = 0; i < MAX_COROUTINE_COUNT; i++) {
		auto ptr = shared_ptr<Coroutine>(new Coroutine(i));
		_lst_co.push_back(ptr);
		_lst_free.push_back(ptr);
	}

	for (int i = 0; i < CO_THREAD_COUNT; i++) {
		_threads.emplace_back([] {
			auto ptr = shared_ptr<CoSchedule>(new CoSchedule);
			ptr->run();
		});
	}

	_timer = new CoTimer;
	_timer->init(CO_TIMER_THREAD_COUNT);
}

CoManager::~CoManager()
{
//	_is_stop = true;
	_is_stop.store(true);
	_cv.notify_all();
	for (auto& item : _threads) {
		item.join();
	}
	delete _timer;
	_timer = NULL;
}

bool CoManager::create(const function<void()>& func)
{
	auto co = create_with_co(func);
	return co ? true : false;
}

shared_ptr<Coroutine> CoManager::create_with_co(const function<void()>& func)
{
	auto co = get_free_co();
	if (co) {
		co->_state = RUNNABLE;
		co->_func  = func;
		add_ready_co(co);
	}
	return co;
}

void CoManager::switch_to_main(const function<void()>& do_after_switch_func)
{
	if (is_in_co_env()) {
		g_schedule->switch_to_main(do_after_switch_func);
	} else {
		THROW_EXCEPTION(CO_ERROR_NOT_IN_CO_ENV, "");
	}
}

void CoManager::yield()
{
	if (is_in_co_env()) {
		g_schedule->yield();
	} else {
		sched_yield();
	}
}

void CoManager::sleep(unsigned int sec)
{
	sleep_ms(sec * 1000);
}

void CoManager::sleep_ms(unsigned int msec)
{
	if (!is_in_co_env()) {
		usleep(msec * 1000);
		return ;
	}

	auto co = get_running_co();
	co->_state = SUSPEND;
	co->_suspend_state = SUSPEND_SLEEP;

	{
		lock_guard<mutex> lock(_mutex);
		_map_suspend[co->_id] = co;
	}

	_timer->set(msec, [this, co] {
		add_ready_co(co);
	}, false);

	g_schedule->switch_to_main();
}

CoTimerId CoManager::set_timer(size_t delay_ms, const std::function<void()>& func)
{
	return _timer->set(delay_ms, func);
}

int CoManager::cancel_timer(CoTimerId& id)
{
	return _timer->cancel(id);
}

CoTimerState CoManager::get_timer_state(const CoTimerId& id)
{
	return _timer->get_state(id);
}

shared_ptr<Coroutine> CoManager::get_co(int id)
{
	assert(id >= 0);
	assert(id < (int)_lst_co.size());
	return _lst_co[id];
}

shared_ptr<Coroutine> CoManager::get_free_co()
{
	shared_ptr<Coroutine> co;
	lock_guard<mutex> lock(_mutex);
	co = _lst_free.front();
	_lst_free.pop_front();
	return co;
}

shared_ptr<Coroutine> CoManager::get_running_co()
{
	shared_ptr<Coroutine> co;
	auto cid = g_schedule->_running_id;
	if (cid >= 0 && cid < (int)_lst_co.size()) {
		co = _lst_co[cid];
	}
	return co;
}

shared_ptr<Coroutine> CoManager::get_ready_co()
{
	shared_ptr<Coroutine> co;

	unique_lock<mutex> lock(_mutex);

	_cv.wait(lock, [this] {return _lst_ready.size() > 0 || _is_stop.load();});

	if (_lst_ready.size() > 0) {
		co = _lst_ready.front();
		_lst_ready.pop_front();
	}

	lock.unlock();

	return co;
}

void CoManager::add_free_co(const shared_ptr<Coroutine>& co)
{
	lock_guard<mutex> lock(_mutex);
	_lst_free.push_back(co);
}

void CoManager::add_ready_co(const shared_ptr<Coroutine>& co, bool priority)
{
	{
		lock_guard<mutex> lock(_mutex);
		if (priority) {
			_lst_ready.push_front(co);
		} else {
			_lst_ready.push_back(co);
		}
	}
	_cv.notify_one();
}

void CoManager::add_suspend_co(const shared_ptr<Coroutine>& co)
{
	lock_guard<mutex> lock(_mutex);
	_map_suspend[co->_id] = co;	
}

bool CoManager::resume_co(int id)
{
	shared_ptr<Coroutine> co;
	{
		lock_guard<mutex> lock(_mutex);
		auto iter = _map_suspend.find(id);
		if (iter == _map_suspend.end()) {
			return false;
		}
		_map_suspend.erase(id);

		co = iter->second;
		co->_state = RUNNABLE;
		co->_suspend_state = SUSPEND_NONE;
	}
	add_ready_co(co);
	return true;
}

void CoManager::co_run(int id)
{
	auto co = g_manager.get_co(id);

	try {
		co->_func();
	} catch (CoException& ex) {
		printf(
			"framework exception, code:%d, file:%s, line:%d\n", 
			ex.code(), 
			ex.file().c_str(), 
			ex.line()
		);
	}

	co->_state = FREE;
	co->_init  = false;
	
	swapcontext(&(co->_ctx), &(g_schedule->_main_ctx));
}
