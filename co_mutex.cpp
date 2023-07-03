#include "co.h"
#include "co_mutex.h"
#include "utils.h"
#include <assert.h>

CoMutex::CoMutex()
{
	_value = 0;
}

CoMutex::~CoMutex()
{
	printf("~CoMutex, ptr:%p\n", this);
}

void CoMutex::lock()
{
	auto is_wake_up = false;
	auto co = g_manager.get_running_co();
	assert(co);

	do {
		int expect = 0;
		if (_value.compare_exchange_strong(expect, 1)) {
			_lock_co = co;
			assert(_lock_co);
			return ;
		}

		if (!_value.load()) {
			continue ;
		}

		co->_state = SUSPEND;
		co->_suspend_state = SUSPEND_LOCK;
		is_wake_up ? _block_list.push_front(co) : _block_list.push_back(co);
		g_manager.add_suspend_co(co);
		g_schedule->switch_to_main();

		is_wake_up = true;

	} while (1);
}

void CoMutex::unlock()
{
	if (!_value.load()) {
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

	_lock_co = nullptr;
	_value.store(0);

	if (_block_list.size() > 0) {

		auto resume_co = _block_list.front();
		_block_list.pop_front();

	//	assert(g_manager.remove_suspend(resume_co->_id));

	//	resume_co->_state = RUNNABLE;
	//	resume_co->_suspend_state = SUSPEND_NONE;

	//	g_manager.add_ready_co(resume_co);

		auto ret = g_manager.resume_co(resume_co->_id);
		assert(ret);
	}
}
