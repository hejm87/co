#include <unistd.h>
#include "co.h"

CoManager g_manager;

thread_local shared_ptr<CoSchedule> g_schedule;

CoSchedule::CoSchedule()
{
	_running_id = -1;
}

CoSchedule::~CoSchedule()
{
	printf("~CoSchedule");
}

void CoSchedule::run()
{
	g_schedule = shared_from_this();
	printf("co_schedule set ptr, tid:%d\n", gettid());
	while (!g_manager._is_stop) {

		process();

		if (!g_schedule->_lst_ready.size()) {
			usleep(1000);
			continue ;
		}

		auto t = g_schedule->_lst_ready.front();	
		g_schedule->_lst_ready.erase(g_schedule->_lst_ready.begin());

		g_schedule->_running_id = t->_id;

		getcontext(&(t->_ctx));

		t->_ctx.uc_stack.ss_sp = t->_stack;
		t->_ctx.uc_stack.ss_size  = DEFAULT_STACK_SZIE;
		t->_ctx.uc_stack.ss_flags = 0;
		t->_ctx.uc_link = &(g_schedule->_main_ctx);

    	makecontext(&(t->_ctx), (void (*)(void))(CoManager::co_run), 1, t->_id);

		swapcontext(&(g_schedule->_main_ctx), &(t->_ctx));
	}
	printf("co_schedule thread end, tid:%d\n", gettid());
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
	_is_stop = true;
	for (auto& item : _threads) {
		item.join();
	}
	printf("~CoManager\n");
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
	}
	return co;	
}

void CoManager::add_free_co(const vector<shared_ptr<Coroutine>>& cos)
{
	lock_guard<mutex> lock(_mutex);
	if (cos.size() > 0) {
		_lst_free.insert(_lst_free.end(), cos.begin(), cos.end());
	}
}

void CoManager::co_run(int id)
{
	auto ptr = g_manager.get_co(id);

	printf("++++++++++++ uthread_func, tid:%d, cid:%d, beg\n", gettid(), ptr->_id);
	ptr->_func();
	printf("------------ uthread_func, tid:%d, cid:%d, end\n", gettid(), ptr->_id);

	ptr->_state = FREE;
	
	g_schedule->_lst_free.push_back(ptr);
	g_schedule->_running_id = -1;
}
