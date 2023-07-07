#include <unistd.h>
#include <assert.h>
#include "co.h"
#include "co_timer.h"
#include "utils.h"

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
	printf("~CoSchedule\n");
}

void CoSchedule::run()
{
	g_schedule = shared_from_this();

	getcontext(&_main_ctx);

	while (!g_manager._is_stop) {

		process();

		if (!_lst_ready.size()) {
			usleep(1000);
			continue ;
		}

		auto co = _lst_ready.front();	
		_lst_ready.erase(_lst_ready.begin());

		_running_id = co->_id;

		if (!co->_init) {

		//	printf("[%s] ++++++++++ co_schedule, schedule1, tid:%d, cid:%d\n", date_ms().c_str(), gettid(), co->_id);

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
			printf("[%s] ++++++++++ co_schedule, co_exit, tid:%d, cid:%d\n", date_ms().c_str(), gettid(), co->_id);
			g_manager.add_free_co(co);
		} else if (co->_state == RUNNABLE) {
			printf("[%s] ++++++++++ co_schedule, co_yield, tid:%d, cid:%d\n", date_ms().c_str(), gettid(), co->_id);
			g_manager.add_ready_co(co);
		} else if (co->_state == SUSPEND) {
			printf("[%s] ++++++++++ co_schedule, co_suspend, tid:%d, cid:%d\n", date_ms().c_str(), gettid(), co->_id);
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

void CoSchedule::resume(shared_ptr<Coroutine> co)
{
	_lst_resume.push_back(co);
}

void CoSchedule::switch_to_main(const function<void()>& do_after_switch_func)
{
	assert(_running_id != -1);
	_do_after_switch_func = do_after_switch_func;

	auto co = g_manager.get_co(_running_id);
	swapcontext(&(co->_ctx), &_main_ctx);
}

void CoSchedule::process()
{
	// 将空闲协程放回g_manager
	g_manager.add_free_co(_lst_free);
	_lst_free.clear();

	// 将待resume协程放回g_manager
	for (auto& co : _lst_resume) {
		g_manager.add_ready_co(co);
	}
	_lst_resume.clear();

	// 从g_manager获取就绪协程
	auto co = g_manager.get_ready_co();
	if (co) {
		_lst_ready.push_back(co);
	}
}

CoManager::CoManager()
{
	for (int i = 0; i < MAX_COROUTINE_SIZE; i++) {
		auto ptr = shared_ptr<Coroutine>(new Coroutine(i));
		_lst_co.push_back(ptr);
		_lst_free.push_back(ptr);
	}

	for (int i = 0; i < CO_THREAD_SIZE; i++) {
		_threads.emplace_back([] {
			auto ptr = shared_ptr<CoSchedule>(new CoSchedule);
			ptr->run();
		});
	}
}

CoManager::~CoManager()
{
	printf("~CoManager, beg\n");
	_is_stop = true;
	for (auto& item : _threads) {
		item.join();
	}
	printf("~CoManager, end\n");
}

bool CoManager::create(const function<void()>& func)
{
	lock_guard<mutex> lock(_mutex);

	if (!_lst_free.size()) {
		return false;
	}

	auto co = _lst_free.front();	
	_lst_free.erase(_lst_free.begin());
	_lst_ready.push_back(co);

	co->_state = RUNNABLE;
	co->_func  = func;

	return true;
}

shared_ptr<Coroutine> CoManager::create_with_co(const function<void()>& func)
{
	shared_ptr<Coroutine> co;

	lock_guard<mutex> lock(_mutex);

	if (_lst_free.size()) {

		co = _lst_free.front();

		_lst_free.erase(_lst_free.begin());
		_lst_ready.push_back(co);

		co->_state = RUNNABLE;	
		co->_func  = func;
	}
	return co;
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
		_lst_sleep.insert(make_pair(now_ms() + msec, co->_id));
	}

	g_schedule->switch_to_main();
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
	// 获取定时器任�?
	auto expires = Singleton<CoTimer>::get_instance()->get_expires();
	for (auto& func : expires) {
		if (!create(func)) {
			Singleton<CoTimer>::get_instance()->set(0, func);
			break ;
		}
	}

//	if (expires.size() > 0) {
//		printf("[%s] +++++++++++++ co_manager, get_ready_co, expires.size:%ld\n", date_ms().c_str(), expires.size());
//	}

	shared_ptr<Coroutine> co;

	lock_guard<mutex> lock(_mutex);

	// 唤醒睡眠协程 
	auto now = now_ms();
	auto beg_iter = _lst_sleep.begin();
	if (_lst_sleep.size() > 0 && beg_iter->first <= now) {
		auto end_iter = _lst_sleep.lower_bound(now);
		for (auto iter = beg_iter; iter != end_iter; iter++) {
			_lst_ready.push_front(_lst_co[iter->second]);
		}
		_lst_sleep.erase(beg_iter, end_iter);
	}

	// 获取�?一�?就绪协程
	if (_lst_ready.size() > 0) {
		co = _lst_ready.front();
		_lst_ready.erase(_lst_ready.begin());
	//	printf("[%s] +++++++++++++ co_manager, get_ready_co, cid:%d\n", date_ms().c_str(), co->_id);
	}
	return co;
}

void CoManager::add_free_co(const shared_ptr<Coroutine>& co)
{
	lock_guard<mutex> lock(_mutex);
	_lst_free.push_back(co);
}

void CoManager::add_free_co(const vector<shared_ptr<Coroutine>>& cos)
{
	lock_guard<mutex> lock(_mutex);
	if (cos.size() > 0) {
		_lst_free.insert(_lst_free.end(), cos.begin(), cos.end());
	}
}

void CoManager::add_ready_co(const shared_ptr<Coroutine>& co, bool priority)
{
	lock_guard<mutex> lock(_mutex);
	if (priority) {
		_lst_ready.push_front(co);
	} else {
		_lst_ready.push_back(co);
	}
}

void CoManager::add_suspend_co(const shared_ptr<Coroutine>& co)
{
	lock_guard<mutex> lock(_mutex);
	_map_suspend[co->_id] = co;	
}

bool CoManager::resume_co(int id)
{
	lock_guard<mutex> lock(_mutex);
	auto iter = _map_suspend.find(id);
	if (iter == _map_suspend.end()) {
		return false;
	}
	_map_suspend.erase(id);

	auto co = iter->second;

	co->_state = RUNNABLE;
	co->_suspend_state = SUSPEND_NONE;

	if (g_schedule) {
		g_schedule->resume(co);
	} else {
		_lst_ready.push_front(co);
	}
	return true;
}

mutex* CoManager::get_locker()
{
	return &_mutex;
}

void CoManager::co_run(int id)
{
	auto co = g_manager.get_co(id);

	co->_func();

	co->_state = FREE;
	co->_init  = false;
	
	swapcontext(&(co->_ctx), &(g_schedule->_main_ctx));
}
