#include "co.h"
#include "co_mutex.h"
#include "utils.h"
#include <assert.h>

CoMutex::CoMutex()
{
	_co_sem = new CoSemaphore(1);
}

CoMutex::~CoMutex()
{
	delete _co_sem;
}

void CoMutex::lock()
{
	_co_sem->wait();
}

void CoMutex::unlock()
{
	lock_guard<mutex> lock(_mutex);
	if (_co_sem->get_value() > 0) {
		THROW_EXCEPTION(CO_ERROR_UNLOCK_ILLEGAL, "");
	}
	_co_sem->signal();
}
