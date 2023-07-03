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

	do {
		int expect = 0;
		if (_value.compare_exchange_strong(expect, 1)) {
			return ;
		}

		if (!_value.load()) {
			continue ;
		}

		shared_ptr<Coroutine> co;
		if (is_in_co_env()) {
			co = g_manager.get_running_co();
		} else {
			co = g_manager.create_with_co([this] {
				_cv_no_co_env.notify_one();
			});
		}
		assert(co);
		
		co->_state = SUSPEND;
		co->_suspend_state = SUSPEND_LOCK;

		is_wake_up ? _block_list.push_front(co) : _block_list.push_back(co);

		g_manager.add_suspend_co(co);
		g_schedule->switch_to_main();

		if (is_in_co_env()) {
			g_schedule->switch_to_main();
		} else {
			unique_lock<mutex> lock(_mutex_no_co_env);
			_cv_no_co_env.wait(lock, [] {return true;});
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

	if (_block_list.size() > 0) {

		auto resume_co = _block_list.front();
		_block_list.pop_front();

		auto ret = g_manager.resume_co(resume_co->_id);
		assert(ret);
	}
}
