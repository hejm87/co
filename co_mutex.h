#ifndef __CO_MUTEX_H__
#define __CO_MUTEX_H__

#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include <list>

using namespace std;

class Coroutine;

class CoMutex
{
public:
	CoMutex();
	CoMutex(const CoMutex&) = delete;

	~CoMutex();

	CoMutex& operator=(const CoMutex&) = delete;

	void lock();

//	bool try_lock();
	
	void unlock();

private:
	atomic<int> _value;
//	shared_ptr<Coroutine> _lock_co;
	list<shared_ptr<Coroutine>> _block_list;

	mutex _mutex_no_co_env;
	condition_variable _cv_no_co_env;
};

//template <class T>
//class CoLockGuard
//{
//public:
//	CoLockGuard(const CoLockGuard&) = delete;
//
//	explicit CoLockGuard(T& obj) {
//		_obj = obj;
//		_obj.lock();
//	}
//
//	~CoLockGuard() {
//		_obj.unlock();
//	}
//
//private:
//	T _obj;
//};

#endif
