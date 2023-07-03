#ifndef __CO_CHANNEL_H__
#define __CO_CHANNEL_H__

#include "co.h"
#include "co_mutex.h"
#include "utils.h"

template <class T>
class CoChannel
{
public:
	CoChannel(size_t size = 0) {
		printf("!!!!!!!!!!!!!! CoChannel(), ptr:%p\n", this);
		_closed = false;
		_max_size = size;
	}

	~CoChannel() {
		printf("!!!!!!!!!!!!!! ~CoChannel(), ptr:%p\n", this);
		close();
	}

	void close() {
		_mutex.lock();
		if (!_closed) {
			_mutex.unlock();
		}
		_closed = false;
		_mutex.unlock();

		for (auto& item : _lst_send_waits) {
			g_manager.resume_co(item->_id);
		}

		for (auto& item : _lst_recv_waits) {
			g_manager.resume_co(item->_id);
		}
		_queue.clear();
	}

	void operator>>(T& obj) {
		{
			_mutex.lock();
			if (_closed) {
				THROW_EXCEPTION("channel close");
			}
			_mutex.unlock();
		}
		if (!_max_size) {
			recv_without_cache(obj);
		} else {
			;
		}
	}

	void operator<<(const T& obj) {
		{
			_mutex.lock();
			if (_closed) {
				THROW_EXCEPTION("channel close");
			}
			_mutex.unlock();
		}
		if (!_max_size) {
			send_without_cache(obj);
		} else {
			;
		}
	}

	void send_without_cache(const T& obj) {
		_mutex.lock();
		if (_lst_recv_waits.size() > 0) {

			printf("[%s] cccccccccc, send_without_cache, path1, tid:%d, cid:%d\n", date_ms().c_str(), gettid(), getcid());

			auto co = _lst_recv_waits.front();
			_lst_recv_waits.pop_front();

			co->_channel_param.param = obj;
			printf(
				"[%s] cccccccccc, send_without_cache, path1, tid:%d, cid:%d, set obj, resume_cid:%d\n", 
				date_ms().c_str(), 
				gettid(), 
				getcid(), 
				co->_id
			);

			_mutex.unlock();

			g_manager.resume_co(co->_id);
		} else {

			printf("[%s] cccccccccc, send_without_cache, path2, tid:%d, cid:%d\n", date_ms().c_str(), gettid(), getcid());

			auto cur_co = g_manager.get_running_co();
			assert(cur_co);

			cur_co->_state = SUSPEND;
			cur_co->_channel_param.type = CHANNEL_BLOCK_SEND;
			cur_co->_channel_param.param = obj;

			_lst_send_waits.push_back(cur_co);

			printf(
				"[%s] cccccccccc, send_without_cache, path2, tid:%d, cid:%d, suspend to wait\n", 
				date_ms().c_str(), 
				gettid(), 
				getcid()
			);

			g_manager.add_suspend_co(cur_co);

			_mutex.unlock();

			g_schedule->switch_to_main();

			printf(
				"[%s] cccccccccc, send_without_cache, path2, tid:%d, cid:%d, wake up\n", 
				date_ms().c_str(),
				gettid(),
				getcid()
			);

			_mutex.lock();
			if (_closed) {
				_mutex.unlock();
				THROW_EXCEPTION("channel close");
			}
			_mutex.unlock();

			cur_co->_channel_param.type = CHANNEL_BLOCK_NULL;
			cur_co->_channel_param.param.Reset();
		}
	}

	void recv_without_cache(T& obj) {
		_mutex.lock();
		if (_lst_send_waits.size() > 0) {

			printf(
				"[%s] cccccccccc, recv_without_cache, path1, tid:%d, cid:%d, get obj now\n", 
				date_ms().c_str(),
				gettid(),
				getcid()
			);

			auto co = _lst_send_waits.front();
			_lst_send_waits.pop_front();

			if (co->_channel_param.type != CHANNEL_BLOCK_SEND) {
				THROW_EXCEPTION("channel inner exception, type:%d", (int)co->_channel_param.type);
			}

			obj = co->_channel_param.param.AnyCast<T>();
			co->_channel_param.param.Reset();

			printf(
				"[%s] cccccccccc, recv_without_cache, path1, tid:%d, cid:%d, resume cid:%d\n", 
				date_ms().c_str(),
				gettid(),
				getcid(),
				co->_id
			);

			_mutex.unlock();

			g_manager.resume_co(co->_id);
		} else {

			printf(
				"[%s] cccccccccc, recv_without_cache, path2, tid:%d, cid:%d, get obj\n", 
				date_ms().c_str(),
				gettid(),
				getcid()
			);

			auto cur_co = g_manager.get_running_co();

			cur_co->_state = SUSPEND;
			cur_co->_channel_param.type = CHANNEL_BLOCK_RECV;
			cur_co->_channel_param.param.Reset();

			_lst_recv_waits.push_back(cur_co);

			g_manager.add_suspend_co(cur_co);

			_mutex.unlock();

			printf(
				"[%s] cccccccccc, recv_without_cache, path2, tid:%d, cid:%d, suspend to wait\n", 
				date_ms().c_str(),
				gettid(),
				getcid()
			);

			g_schedule->switch_to_main();

			_mutex.lock();

			if (_closed) {
				_mutex.unlock();
				THROW_EXCEPTION("channel close");
			}

			_mutex.unlock();

			printf(
				"[%s] cccccccccc, recv_without_cache, path2, tid:%d, cid:%d, wake up\n", 
				date_ms().c_str(),
				gettid(),
				getcid()
			);

			if (cur_co->_channel_param.param.IsNull()) {
				THROW_EXCEPTION("channel inner exception");
			}

			obj = cur_co->_channel_param.param.AnyCast<T>();
			cur_co->_channel_param.param.Reset();
		}
	}

private:
	bool _closed;

	size_t _max_size;

	list<shared_ptr<Coroutine>> _lst_send_waits;	
	list<shared_ptr<Coroutine>> _lst_recv_waits;	

	list<T> _queue;

	CoMutex _mutex;
};

#endif
