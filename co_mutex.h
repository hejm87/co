#ifndef __CO_MUTEX_H__
#define __CO_MUTEX_H__

#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include <list>
#include <set>

#include "common/semaphore.h"

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

	void unlock();

private:
	// ������id(29)|�Ƿ�Э�̻�������(1)|���ڲ���(1)|�Ƿ�����(1)
	atomic<int> _value;
	list<shared_ptr<Coroutine>> _block_list;

	mutex _mutex;

	semaphore* _sem;

//	mutex _mutex_no_co_env;
//	condition_variable _cv_no_co_env;
//	set<int> _set_flag_no_co_env;
};

#endif
