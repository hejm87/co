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
	CO_TIMER_CANCEL,
};

struct CoTimerInfo;

class CoTimerId
{
friend class CoTimer;
friend class CoTimerInfo;
public:
	CoTimerId() = delete;
//	CoTimerId(const CoTimerId& id) = delete;

	CoTimerId& operator=(const CoTimerId& id) = delete;

//	CoTimerId(CoTimerId&& id) {
//		printf("############# CoTimerId(CoTimerId&& id)\n");
//		swap(_ptr, id._ptr);
//	}

	CoTimerId(const CoTimerId& id) {
		printf("############# CoTimerId(const CoTimerId& id)\n");
		_ptr = id._ptr;
	}

private:
	CoTimerId(shared_ptr<CoTimerInfo> ptr) {
		printf("############# CoTimerId(shared_ptr<CoTimerInfo> ptr)\n");
		_ptr = ptr;
	}

	shared_ptr<CoTimerInfo> _ptr;
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
		ptr->func = func;
		return set(delay_ms, ptr);
	}

	CoTimerId set(size_t delay_ms, shared_ptr<CoTimerInfo> ptr) {
		ptr->active_time = now_ms() + delay_ms;
		ptr->state = CO_TIMER_WAIT;
		{
			lock_guard<mutex> lock(_mutex);
			_map_list_iter[ptr] = _list.insert(make_pair(ptr->active_time, ptr));	
		}
		return CoTimerId(ptr);
	}

	bool cancel(CoTimerId& id) {
		if (!id._ptr) {
			THROW_EXCEPTION(CO_ERROR_INNER_EXCEPTION, "CoTimerId invalid");
		}
		if (id._ptr->state == CO_TIMER_CANCEL) {
			THROW_EXCEPTION(CO_ERROR_INNER_EXCEPTION, "CoTimerId already cancel");
		}
		if (id._ptr->state == CO_TIMER_PROCESS || id._ptr->state == CO_TIMER_FINISH) {
			return false;
		}
		lock_guard<mutex> lock(_mutex);
		auto iter = _map_list_iter.find(id._ptr);
		if (iter == _map_list_iter.end()) {
			return false;
		}
		id._ptr->state = CO_TIMER_CANCEL;
		_list.erase(iter->second);
		_map_list_iter.erase(iter);
		return true;
	}

	shared_ptr<CoTimerInfo> get_expire() {
		shared_ptr<CoTimerInfo> expire;
		lock_guard<mutex> lock(_mutex);
		auto now = now_ms();
		if (_list.size() && now >= _list.begin()->first) {
			expire = _list.begin()->second;
			expire->state = CO_TIMER_READY;
			_map_list_iter.erase(_list.begin()->second);
			_list.erase(_list.begin());
		}
		return expire;
	}

	vector<shared_ptr<CoTimerInfo>> get_expires() {
		vector<shared_ptr<CoTimerInfo>> expires;
		lock_guard<mutex> lock(_mutex);
		auto now = now_ms();
		if (_list.size() && now >= _list.begin()->first) {
			auto beg_iter = _list.begin();
			auto end_iter = _list.lower_bound(now);
			for (auto iter = beg_iter; iter != end_iter; iter++) {
				iter->second->state = CO_TIMER_READY;
				expires.emplace_back(iter->second);
				_map_list_iter.erase(iter->second);
			}
			_list.erase(beg_iter, end_iter);
		}
		return expires;
	}

	CoTimerState get_state(const CoTimerId& id) {
		if (!id._ptr) {
			THROW_EXCEPTION(CO_ERROR_INNER_EXCEPTION, "CoTimerId invalid");
		}
		return id._ptr->state;
	}

private:
	timer_list_t		_list;
	map_timer_list_t	_map_list_iter;	

	mutex	_mutex;
};

#endif
