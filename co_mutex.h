#ifndef __CO_MUTEX_H__
#define __CO_MUTEX_H__

#include <memory>
#include <atomic>
#include <list>

class Coroutine;

class CoMutex
{
public:
	CoMutex();
	CoMutex(const CoMutex&) = delete;

	void lock();

//	bool try_lock();
	
	void unlock();

private:
	atomic<int> _value;
	shared_ptr<Coroutine> _lock_co;
	list<shared_ptr<Coroutine>> _block_list;
};

template <class T>
class CoLockGuard
{
public:
	CoLockGuard(const CoLockGuard&) = delete;

	explicit CoLockGuard(T& obj) {
		_obj = obj;
		_obj.lock();
	}

	~CoLockGuard() {
		_obj.unlock();
	}

private:
	T _obj;
};

#endif
