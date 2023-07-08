#ifndef __CO_TIMER_H__
#define __CO_TIMER_H__

#include <map>
#include <unordered_map>

#include <memory>
#include <mutex>

#include "co.h"
#include "utils.h"

using namespace std;

enum CoTimerState
{
	CO_TIMER_WAIT = 0,
	CO_TIMER_READY,
	CO_TIMER_PROCESS,
	CO_TIMER_FINISH,
};

struct CoTimerInfo;

class CoTimerId
{
friend class CoTimer;
friend class CoTimerInfo;
public:
	CoTimerId() = delete;
	CoTimerId(const CoTimerId& id) = delete;

	CoTimerId& operator=(const CoTimerId& id) = delete;

private:
	CoTimerId(shared_ptr<CoTimerInfo> ptr) {
		_ptr = ptr;
	}

	CoTimerId(CoTimerId&& id) {
		swap(_ptr, id._ptr);
	}

	weak_ptr<CoTimerInfo> _ptr;
};

struct CoTimerInfo 
{
	long				active_time;
	CoTimerState		state;
	function<void()>	func;
};

class CoTimer
{
public:
	typedef multimap<long, shared_ptr<CoTimerInfo>> timer_list_t;
	typedef unordered_map<shared_ptr<CoTimerInfo>, timer_list_t::iterator> map_timer_list_t;

	CoTimerId set(size_t delay_ms, const std::function<void()>& func) {
		auto ptr = shared_ptr<CoTimerInfo>(new CoTimerInfo);
		ptr->active_time = now_ms() + delay_ms;
		ptr->state = CO_TIMER_WAIT;
		ptr->func  = func;
		lock_guard<mutex> lock(_mutex);
		_map_list_iter[ptr] = _list.insert(make_pair(ptr->active_time, ptr));	
		return CoTimerId(ptr);
	}

	bool cancel(const CoTimerId& id) {
		auto ptr = id._ptr.lock();	
		if (!ptr) {
			return false;
		}
		lock_guard<mutex> lock(_mutex);
		auto iter = _map_list_iter.find(ptr);
		if (iter == _map_list_iter.end()) {
			return false;
		}
		_list.erase(iter->second);
		_map_list_iter.erase(iter);
		return true;
	}

	vector<function<void()>> get_expires() {
		vector<function<void()>> expires;
		lock_guard<mutex> lock(_mutex);
		auto now = now_ms();
	//	printf(
	//		"[%s] !!!!!!!!!!!!!!! get_expires, tid:%d, _list.size:%ld, now:%ld\n", 
	//		date_ms().c_str(),
	//		gettid(),
	//		_list.size(),
	//		now
	//	);
		if (_list.size() && now >= _list.begin()->first) {
			auto beg_iter = _list.begin();
			auto end_iter = _list.lower_bound(now);
			for (auto iter = beg_iter; iter != end_iter; iter++) {
				expires.emplace_back(iter->second->func);
				_map_list_iter.erase(iter->second);
			}
			_list.erase(beg_iter, end_iter);
		//	printf(
		//		"[%s] !!!!!!!!!!!!!!! get_expires, tid:%d, size:%ld\n", 
		//		date_ms().c_str(),
		//		gettid(),
		//		expires.size()
		//	);
		}
		return expires;
	}

	CoTimerState get_state(const CoTimerId& id) {
		auto timer_id = id._ptr.lock();
		if (!timer_id) {
			return CO_TIMER_FINISH; 
		}
		lock_guard<mutex> lock(_mutex);
		return timer_id->state;
	}

private:
	timer_list_t		_list;
	map_timer_list_t	_map_list_iter;	

	mutex	_mutex;
};

#endif
