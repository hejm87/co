#include "co.h"
#include "co_timer.h"

CoTimer::CoTimer() {
	_is_init = false;
	_is_set_end = false;
}

CoTimer::~CoTimer() {
	_is_set_end = true;	
	_cv.notify_all();
	for (auto& item : _threads) {
		item.join();
	}
}

void CoTimer::init(size_t thread_num) {
	std::lock_guard<std::mutex> lock(_mutex);
	if (!_is_init) {
		_is_init = true;
		for (int i = 0; i < (int)thread_num; i++) {
			_threads.emplace_back([this]() {
				run();
			});
		}
	}
	return ;
}

CoTimerId CoTimer::set(size_t delay_ms, const std::function<void()>& func, bool deal_with_co) {
	auto ptr = std::shared_ptr<CoTimerInfo>(new CoTimerInfo);
	ptr->active_time  = now_ms() + (long)delay_ms;
    ptr->deal_with_co = deal_with_co;
	ptr->func = func;
	ptr->state.store((int)CO_TIMER_WAIT);

	CoTimerId ti(ptr);
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_map_list_iter[ptr] = _list.insert(make_pair(ptr->active_time, ptr));
	}
	_cv.notify_one();
	return ti;
}

int CoTimer::cancel(const CoTimerId& id) {
	if (!id._ptr) {
		return CO_TIMER_ERROR_TIMERID_INVALID;
	}
	auto state = (CoTimerState)(id._ptr->state.load());
	if (state == CO_TIMER_PROCESS || state == CO_TIMER_FINISH) {
		return CO_TIMER_ERROR_TIMER_CANT_CANCEL;
	}
	if (state == CO_TIMER_CANCEL) {
		return CO_TIMER_ERROR_TIMER_ALREADY_CANCEL;
	}
	lock_guard<mutex> lock(_mutex);
	auto iter = _map_list_iter.find(id._ptr);
	if (iter == _map_list_iter.end()) {
		return CO_TIMER_ERROR_TIMERID_NOT_FOUND;
	}
	id._ptr->state.store((int)CO_TIMER_CANCEL);
	_list.erase(iter->second);
	_map_list_iter.erase(iter);
	return 0;
}

CoTimerState CoTimer::get_state(const CoTimerId& id) {
	if (!id._ptr) {
		return CO_TIMER_UNKNOW;
	}
	auto state = (CoTimerState)(id._ptr->state.load());
	if (state == CO_TIMER_WAIT && id._ptr->active_time <= now_ms()) {
		state = CO_TIMER_READY;
	}
	return state;
}

int CoTimer::size() {
	std::lock_guard<std::mutex> lock(_mutex);
	return (int)_list.size();
}

bool CoTimer::empty() {
	std::lock_guard<std::mutex> lock(_mutex);
	return _list.size() == 0 ? true : false;	
}

void CoTimer::run() {
	while (!_is_set_end) {
		int wait_ms = 0;
		std::shared_ptr<CoTimerInfo> ptr;
		{
			std::lock_guard<std::mutex> lock(_mutex);
			if (!_list.empty()) {
				auto iter = _list.begin();	
				auto delta = iter->first - now_ms();
				if (delta <= 0) {
					ptr = iter->second;
					_map_list_iter.erase(iter->second);
					_list.erase(iter);
				} else {
					wait_ms = delta;
				}
			}
		}

        if (ptr) {
            if (ptr->deal_with_co) {
                do {
                    auto ret = g_manager.create([ptr] {
                        ptr->state.store(CO_TIMER_PROCESS);
                        ptr->func();
                        ptr->state.store(CO_TIMER_FINISH);
                    });
                    if (ret) {
                        break ;
                    }
                   // printf(
                   //     "[%s] timer, tid:%d, coroutine resource not enough, wait a minute\n", 
                   //     date_ms().c_str(), 
                   //     gettid()
                   // );
                    usleep(1000);
                } while (1);
            } else {
                ptr->state.store(CO_TIMER_PROCESS);
                ptr->func();
                ptr->state.store(CO_TIMER_FINISH);
            }
            continue ;
        }

		std::unique_lock<std::mutex> lock(_mutex);
		if (wait_ms == 0) {
			_cv.wait(lock, [this] {
				return !_list.empty() || _is_set_end;
			});
		} else {
			_cv.wait_for(lock, chrono::milliseconds(wait_ms), [this] {
				return !_list.empty() || _is_set_end;
			});
		}
	}
}
