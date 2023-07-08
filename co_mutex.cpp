#include "co.h"
#include "co_mutex.h"
#include "utils.h"
#include <assert.h>

CoMutex::CoMutex()
{
	_value = 0;
	_sem = new Semaphore();
}

CoMutex::~CoMutex()
{
	printf("~CoMutex, ptr:%p\n", this);
	delete _sem;
}

void CoMutex::lock()
{
	auto is_wake_up = false;
	do {
		int expect = 0;
		if (_value.compare_exchange_strong(expect, 1)) {
		//	printf("[%s] mmmmmmmmmmmmmmm, cid:%d, lock succeed\n", date_ms().c_str(), getcid());	
			return ;
		}

		_mutex.lock();
		if (!_value.load()) {
			_mutex.unlock();
			continue ;
		}

		shared_ptr<Coroutine> co;
		if (is_in_co_env()) {
			co = g_manager.get_running_co();
		} else {
			co = g_manager.get_free_co();
			co->_func = [this] {
				_sem->signal();
			};
		}
		assert(co);

		co->_state = SUSPEND;
		co->_suspend_state = SUSPEND_LOCK;

		printf("[%s] mmmmmmmmmmmmmmm, CoMutex::lock, _block_list.push_xxxx, cid:%d\n", date_ms().c_str(), co->_id);
		is_wake_up ? _block_list.push_front(co) : _block_list.push_back(co);

		if (is_in_co_env()) {
			g_schedule->switch_to_main([this] {
				_mutex.unlock();
			});
		} else {
			g_manager.add_suspend_co(co);
			_mutex.unlock();
			_sem->wait();
		}

		is_wake_up = true;

	} while (1);
}

void CoMutex::unlock()
{
	if (!_value.load()) {
		return ;
	}

	_value.store(0);
	{
		lock_guard<mutex> lock(_mutex);
		if (_value.load()) {
			return ;
		}
		if (_block_list.size() > 0) {
			auto resume_co = _block_list.front();
			_block_list.pop_front();
			printf("[%s] mmmmmmmmmmmmm, CoMutex::unlock, ready to resume_co cid:%d\n", date_ms().c_str(), resume_co->_id);
			auto ret = g_manager.resume_co(resume_co->_id);
			if (!ret) {
				printf("[%s] mmmmmmmmmmmmm, CoMutex::unlock, resume_co fail, cid:%d\n", date_ms().c_str(), resume_co->_id);
			}
			assert(ret);
		}
	}
}
