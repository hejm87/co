#ifndef __CO_TIMER_H__
#define __CO_TIMER_H__

#include <unistd.h>

#include <map>
#include <unordered_map>
#include <vector>

#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <functional>

enum CoTimerState
{
	CO_TIMER_UNKNOW = -1,
	CO_TIMER_WAIT,
	CO_TIMER_READY,
	CO_TIMER_PROCESS,
	CO_TIMER_FINISH,
	CO_TIMER_CANCEL,
};

const int CO_TIMER_ERROR_TIMERID_INVALID = 1;
const int CO_TIMER_ERROR_TIMERID_NOT_FOUND = 2;
const int CO_TIMER_ERROR_TIMER_CANT_CANCEL = 3;
const int CO_TIMER_ERROR_TIMER_ALREADY_CANCEL = 4;

class CoTimer;

struct CoTimerInfo
{
	long				active_time;
    bool                deal_with_co;
	atomic<int>			state;
	function<void()>	func;
};

class CoTimerId
{
friend class CoTimer;
public:
	CoTimerId() = delete;
	CoTimerId& operator=(const CoTimerId& id) = delete;

protected:
	CoTimerId(shared_ptr<CoTimerInfo> ptr) {
		_ptr = ptr;
	}

	std::shared_ptr<CoTimerInfo> _ptr;
};

class CoTimer
{
public:
	typedef std::multimap<long, std::shared_ptr<CoTimerInfo>>	timer_list_t;
	typedef std::unordered_map<std::shared_ptr<CoTimerInfo>, timer_list_t::iterator>	map_timer_list_t;

	CoTimer();
    ~CoTimer();

	void init(size_t thread_num);

	CoTimerId set(size_t delay_ms, const std::function<void()>& func, bool deal_with_co = true);

    int cancel(const CoTimerId& id);

	CoTimerState get_state(const CoTimerId& id);

	int size();

	bool empty(); 

private:
	void run(); 

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