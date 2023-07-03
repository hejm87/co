#ifndef __TIMER_H__
#define __TIMER_H__

#include <unistd.h>

#include <map>
#include <unordered_map>
#include <vector>

#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <functional>

#include "any.h"
//#include "helper.h"

class Timer;

typedef std::function<void()> timer_func_t;

class TimerId
{
friend class Timer;
private:
	std::weak_ptr<timer_func_t> _ptr;
};

class Timer
{
public:
	typedef std::multimap<long, std::shared_ptr<timer_func_t>>	timer_list_t;
	typedef std::unordered_map<std::shared_ptr<timer_func_t>, timer_list_t::iterator>	map_timer_list_t;

	Timer() {
		_is_init = false;
		_is_set_end = false;
	}

    ~Timer() {
		_is_set_end = true;	
		_cv.notify_all();
		for (auto& item : _threads) {
			item.join();
		}
	}

	bool init(int thread_num) {
		std::lock_guard<std::mutex> lock(_mutex);
		if (_is_init) {
			return true;
		}
		_is_init = true;
		for (int i = 0; i < thread_num; i++) {
			_threads.emplace_back([this]() {
				run();
			});
		}
	}
	
	TimerId set(int delay_ms, const std::function<void()>& func) {
		auto ptr = std::shared_ptr<timer_func_t>(new timer_func_t(func));
		std::lock_guard<std::mutex> lock(_mutex);
		_map_list_iter[ptr] = _list.insert(make_pair(now_ms() + delay_ms, ptr));
		TimerId ti;
		ti._ptr = ptr;
		_cv.notify_one();
		return ti;
	}

    bool cancel(const TimerId& id) {
		std::lock_guard<std::mutex> lock(_mutex);
		auto ptr = id._ptr.lock();
		if (!ptr) {
			return false;
		}
		auto iter = _map_list_iter.find(ptr);
		if (iter == _map_list_iter.end()) {
			return false;
		}
		_list.erase(iter->second);
		_map_list_iter.erase(iter);
		return true;
	}

	int size() {
		std::lock_guard<std::mutex> lock(_mutex);
		return (int)_list.size();
	}

	bool empty() {
		std::lock_guard<std::mutex> lock(_mutex);
		return _list.size() == 0 ? true : false;	
	}

private:
	void run() {
		while (!_is_set_end) {
			int wait_ms = 0;
			std::shared_ptr<timer_func_t> ptr;
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
				(*ptr)();
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

private:
	timer_list_t		_list;
	map_timer_list_t	_map_list_iter;

	std::mutex	_mutex;
	std::condition_variable		_cv;
	std::vector<std::thread>	_threads;

	bool	_is_init;
	bool	_is_set_end;
};

#endif
