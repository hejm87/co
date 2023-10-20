#ifndef __CO_CHANNEL_H__
#define __CO_CHANNEL_H__

#include "co.h"
#include "co_mutex.h"
#include "common/common_utils.h"
#include "common/semaphore.h"
#include "co_semaphore.h"
#include "co_exception.h"

template <class T>
class CoChannelSendItem
{
public:
	CoChannelSendItem(const T& obj) {
		this->code = 0;
		this->obj = obj;
	}
public:
	int code;
	T obj;
	CoSemaphore sem;
};

template <class T>
class CoChannelBase
{
public:
	virtual ~CoChannelBase() {;}

	virtual void set_obj(const T& obj) = 0;
	virtual T get_obj() = 0;	

	virtual void close() = 0;

	virtual bool is_close() = 0;
};

template <class T>
class CoChannelSync : public CoChannelBase<T>
{
public:
	CoChannelSync() {
		_closed = false;
		_recv_wait_count = 0;
	}

	~CoChannelSync() {
		close();
	}

	bool is_close() {
		bool closed;
		_co_mutex.lock();
		closed = _closed;
		_co_mutex.unlock();
		return closed;
	}

	void close() {
		_co_mutex.lock();
		if (_closed) {
			_co_mutex.unlock();
			return ;
		}

		_closed = true;

		for (auto& item : _lst_send) {
			item->code = -1;
			item->sem.signal();
		}
		_lst_send.clear();

		if (_co_sem_recv.get_value() < 0) {
			int count = _co_sem_recv.get_value() * -1;
			for (int i = 0; i < count; i++) {
				_co_sem_recv.signal();
			}
		}

		_co_mutex.unlock();
	}

	void set_obj(const T& obj) {
		_co_mutex.lock();
		if (_closed) {
			_co_mutex.unlock();
			THROW_EXCEPTION(CO_ERROR_CHANNEL_CLOSE, "channel close");
		}

		auto send_ptr = shared_ptr<CoChannelSendItem<T>>(new CoChannelSendItem<T>(obj));
		_lst_send.push_back(send_ptr);
	//	printf(
	//		"[%s] ccccccccccccccc, set_obj, tid:%d, cid:%d, _lst_send.push_back\n", 
	//		date_ms().c_str(),
	//		gettid(),
	//		getcid()
	//	);
		if (_recv_wait_count > 0) {
		//	printf(
		//		"[%s] ccccccccccccccc, set_obj, tid:%d, cid:%d, signal receiver\n", 
		//		date_ms().c_str(),
		//		gettid(),
		//		getcid()
		//	);
			_recv_wait_count--;
			_co_sem_recv.signal();
		}
		_co_mutex.unlock();

	//	printf(
	//		"[%s] ccccccccccccccc, set_obj, tid:%d, cid:%d, wait\n", 
	//		date_ms().c_str(),
	//		gettid(),
	//		getcid()
	//	);
		send_ptr->sem.wait();
	//	printf(
	//		"[%s] ccccccccccccccc, set_obj, tid:%d, cid:%d, wait finish\n", 
	//		date_ms().c_str(),
	//		gettid(),
	//		getcid()
	//	);
		if (send_ptr->code) {
			THROW_EXCEPTION(CO_ERROR_CHANNEL_CLOSE, "channel close");
		}
	}

	T get_obj() {
		_co_mutex.lock();
		if (_closed) {
			_co_mutex.unlock();
			THROW_EXCEPTION(CO_ERROR_CHANNEL_CLOSE, "channel close");
		}

		while (!_lst_send.size()) {
			if (_closed) {
				_co_mutex.unlock();
				THROW_EXCEPTION(CO_ERROR_CHANNEL_CLOSE, "channel close");
			}
			_recv_wait_count++;
			_co_mutex.unlock();
			_co_sem_recv.wait();
			_co_mutex.lock();
		}

		auto ptr = _lst_send.front();
		_lst_send.pop_front();

		_co_mutex.unlock();

		ptr->sem.signal();
		return ptr->obj;
	}

private:
	bool _closed;
	int  _recv_wait_count;

	CoMutex _co_mutex;
	CoSemaphore _co_sem_recv;

	list<shared_ptr<CoChannelSendItem<T>>> _lst_send;
};

template <class T>
class CoChannelAsync : public CoChannelBase<T>
{
public:
	CoChannelAsync(size_t size) {
		_send_wait = 0;
		_recv_wait = 0;
		_max_size = size;
		_closed = false;
	}

	~CoChannelAsync() {
		close();
	}

	bool is_close() {
		bool closed;
		_co_mutex.lock();
		closed = _closed;
		_co_mutex.unlock();
		return closed;
	}

	void close() {
		_co_mutex.lock();
		if (_closed) {
			_co_mutex.unlock();
			return ;
		}
		_closed = true;
		for (int i = 0; i < _send_wait; i++) {
			_send_sem.signal();
		}
		for (int i = 0; i < _recv_wait; i++) {
			_recv_sem.signal();
		}
		_co_mutex.unlock();
	}

	void set_obj(const T& obj) {
		do {
			_co_mutex.lock();
			if (_closed) {
				_co_mutex.unlock();
				THROW_EXCEPTION(CO_ERROR_CHANNEL_CLOSE, "channel close");
			}
			// 放置队列操作
			auto cur_size = _queue.size();
			if (cur_size < _max_size) {
				_queue.push_back(obj);
				if (_recv_wait > 0) {
					_recv_wait--;
					_recv_sem.signal();
				}
				_co_mutex.unlock();
			//	printf("[%s] ccccccccccccccc, tid:%d, cid:%d, set_obj by now\n", date_ms().c_str(), gettid(), getcid());
				break ;
			}
			// 放置到发送队列，休眠待唤醒
			_send_wait++;
			_co_mutex.unlock();
		//	printf("[%s] ccccccccccccccc, tid:%d, cid:%d, set_obj by wait\n", date_ms().c_str(), gettid(), getcid());
			_send_sem.wait();
		} while (1);
	}

	T get_obj() {
		do {
			_co_mutex.lock();
			auto cur_size = _queue.size();
			if (_closed && !cur_size) {
				_co_mutex.unlock();
				THROW_EXCEPTION(CO_ERROR_CHANNEL_CLOSE, "channel close");
			}
			if (cur_size > 0) {
				auto obj = _queue.front();
				_queue.pop_front();
				if (!_closed && _send_wait > 0) {
					_send_wait--;
					_send_sem.signal();
				}
				_co_mutex.unlock();
				return obj;
			} else {
				// 放置到接收队列，休眠待唤醒
				_recv_wait++;
				_co_mutex.unlock();
				_recv_sem.wait();
			}
		} while (1);
	}

private:
	bool _closed;

	int _max_size;
	list<T> _queue;
	
	int _send_wait;
	int _recv_wait;

	CoSemaphore _send_sem;
	CoSemaphore _recv_sem;

	CoMutex _co_mutex;
};

template <class T>
class CoChannel
{
public:
	CoChannel(const CoChannel&) = delete;
	CoChannel& operator=(const CoChannel&) = delete;

	CoChannel(size_t size = 0) {
		if (!size) {
			_co_chan = new CoChannelSync<T>();
		} else {
			_co_chan = new CoChannelAsync<T>(size);
		}
	}

	~CoChannel() {
		close();
		delete _co_chan;
		_co_chan = NULL;
	}

	bool is_close() {
		_co_chan->is_close();
	}

	void close() {
		_co_chan->close();
	}

	void operator>>(T& obj) {
		obj = _co_chan->get_obj();
	}

	void operator<<(const T& obj) {
		_co_chan->set_obj(obj);
	}

private:
	CoChannelBase<T>* _co_chan;
};

#endif
