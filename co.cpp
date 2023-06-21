#include <unistd.h>
#include "co.h"

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

			printf("++++++++++ co_schedule, schedule1, tid:%d, cid:%d\n", gettid(), co->_id);

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
			printf("++++++++++ co_schedule, co_exit, tid:%d, cid:%d\n", gettid(), co->_id);
			g_manager.add_free_co(co);
		} else if (co->_state == RUNNABLE) {
			printf("++++++++++ co_schedule, co_yield, tid:%d, cid:%d\n", gettid(), co->_id);
			g_manager.add_ready_co(co);
		} else if (co->_state == SUSPEND) {
			printf("++++++++++ co_schedule, co_suspend, tid:%d, cid:%d\n", gettid(), co->_id);
			g_manager.add_suspend_co(co);
		}
	
		// for test yield, other thread process
	//	usleep(10 * 1000);
	}
	printf("co_schedule, tid:%d, thread end\n", gettid());
}

void CoSchedule::yield()
{
	auto co = g_manager.get_co(_running_id);
	co->_state = RUNNABLE;

	swapcontext(&(co->_ctx), &_main_ctx);
}

void CoSchedule::process()
{
	auto co = g_manager.get_ready_co();
	if (co) {
		_lst_ready.push_back(co);
	}
	g_manager.add_free_co(_lst_free);
	_lst_free.clear();
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

bool CoManager::create(function<void()> func)
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

void CoManager::suspend()
{
	auto co = _lst_co[g_schedule->_running_id];
	co->_state = SUSPEND;

	swapcontext(&(co->_ctx), &(g_schedule->_main_ctx));
}

shared_ptr<Coroutine> CoManager::get_co(int id)
{
	shared_ptr<Coroutine> co;
	if (id < (int)_lst_co.size()) {
		co = _lst_co[id];
	}
	return co;
}

shared_ptr<Coroutine> CoManager::get_ready_co()
{
	shared_ptr<Coroutine> co;
	lock_guard<mutex> lock(_mutex);
	if (_lst_ready.size() > 0) {
		co = _lst_ready.front();
		_lst_ready.erase(_lst_ready.begin());
		printf("co_manager, get_ready_co, cid:%d\n", co->_id);
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

void CoManager::add_ready_co(const shared_ptr<Coroutine>& co)
{
	lock_guard<mutex> lock(_mutex);
	_lst_ready.push_back(co);
}

void CoManager::add_suspend_co(const shared_ptr<Coroutine>& co)
{
	lock_guard<mutex> lock(_mutex);
	_map_suspend[co->_id] = co;	
}

void CoManager::co_run(int id)
{
	auto co = g_manager.get_co(id);

//	printf("++++++++++++ uthread_func, tid:%d, cid:%d, beg\n", gettid(), co->_id);
	co->_func();
//	printf("------------ uthread_func, tid:%d, cid:%d, end\n", gettid(), co->_id);

	co->_state = FREE;
	co->_init  = false;
	
//	g_schedule->_lst_free.push_back(co);
//	g_schedule->_running_id = -1;
//	printf("------------ uthread_func, tid:%d, cid:%d, ready to change\n", gettid(), co->_id);

	swapcontext(&(co->_ctx), &(g_schedule->_main_ctx));
}
