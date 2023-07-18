#ifndef __CO_MUTEX_H__
#define __CO_MUTEX_H__

#include <mutex>
#include "co_semaphore.h"

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
	CoSemaphore* _co_sem;

	mutex _mutex;
};

#endif
