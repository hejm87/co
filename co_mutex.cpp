#include "co.h"
#include "co_mutex.h"
#include "utils.h"
#include <assert.h>

CoMutex::CoMutex()
{
	_value = 0;
}

void CoMutex::lock()
{
	auto is_wake_up = false;
	auto co = g_manager.get_running_co();
	assert(co);

	do {
		int expect = 0;
		if (_value.compare_exchange_strong(expect, 1)) {
			printf("[%s] ------------- tid:%d, cid:%d, lock succeed\n", date_ms().c_str(), gettid(), getcid());
			_lock_co = co;
			assert(_lock_co);
			return ;
		}

		{
			lock_guard<mutex> lock(*(g_manager.get_locker()));
			if (!_value.load()) {
				continue ;
			}

			printf("[%s] ------------- tid:%d, cid:%d, is locked, suspend\n", date_ms().c_str(), gettid(), getcid());
			co->_state = SUSPEND;
			co->_suspend_state = SUSPEND_LOCK;
			is_wake_up ? _block_list.push_front(co) : _block_list.push_back(co);
			g_manager.add_suspend_co(co);
		}
		g_schedule->switch_to_main();
		printf("[%s] ------------- tid:%d, cid:%d, suspend wake up\n", date_ms().c_str(), gettid(), getcid());
		is_wake_up = true;
	} while (1);
}

void CoMutex::unlock()
{
	if (!_value.load()) {
		printf("[%s] ------------- tid:%d, cid:%d, unlock but value not match\n", date_ms().c_str(), gettid(), getcid());
		return ;
	}
	auto cur_co = g_manager.get_running_co();
	assert(cur_co);
	if (_value.load() && _lock_co != cur_co) {
		THROW_EXCEPTION(
			"CoMutex unlock, _value:%d, _lock_co.cid:%d, cur_co.cid:%d", 
			_value.load(),
			_lock_co->_id,
			cur_co->_id
		);
	}

	lock_guard<mutex> lock(*(g_manager.get_locker()));

	_lock_co = nullptr;
	_value.store(0);
	printf("[%s] ------------- tid:%d, cid:%d, unlock\n", date_ms().c_str(), gettid(), getcid());
	if (_block_list.size() > 0) {

		auto resume_co = _block_list.front();
		_block_list.pop_front();

		assert(g_manager.remove_suspend(resume_co->_id));

		resume_co->_state = RUNNABLE;
		resume_co->_suspend_state = SUSPEND_NONE;

		g_manager.add_ready_co(resume_co);
		printf("[%s] ------------- tid:%d, cid:%d, unlock, resume cid:%d\n", date_ms().c_str(), gettid(), getcid(), resume_co->_id);
	}
}
