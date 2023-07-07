#ifndef __CO_CHANNEL_H__
#define __CO_CHANNEL_H__

#include "co.h"
#include "co_mutex.h"
#include "common/semaphore.h"
#include "utils.h"

#include <mutex>
#include <condition_variable>

template <class T>
class CoChannel
{
public:
	CoChannel(size_t size = 0) {
		printf("!!!!!!!!!!!!!! CoChannel(), ptr:%p\n", this);
		_closed = false;
		_max_size = size;
		_sem_send = new semaphore(0);
		_sem_recv = new semaphore(0);
	}

	~CoChannel() {
		printf("!!!!!!!!!!!!!! ~CoChannel(), ptr:%p\n", this);
		close();
		delete _sem_send;
		delete _sem_recv;
	}

	void close() {
		_mutex.lock();
		if (_closed) {
			_mutex.unlock();
			return ;
		}
		_closed = true;
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
		if (!_max_size) {
			recv_without_cache(obj);
		} else {
			;
		}
	}

	void operator<<(const T& obj) {
		if (!_max_size) {
			send_without_cache(obj);
		} else {
			;
		}
	}

	void send_without_cache(const T& obj) {

		_mutex.lock();

		if (_closed) {
			_mutex.unlock();
			THROW_EXCEPTION("channel close");
		}

		shared_ptr<Coroutine> cur_co;
		if (is_in_co_env()) {
			cur_co = g_manager.get_running_co();
		} else {
			cur_co = g_manager.get_free_co();
			cur_co->_func = [this] {
				_sem_send->signal();
			};
		}

		// lock放下面有被lock重置co._state问题，后续需要优化代码
		// 这种显式设置协程状态的模式需要去除
		assert(cur_co);
		cur_co->_state = SUSPEND;
		cur_co->_channel_param.type = CHANNEL_BLOCK_SEND;
		cur_co->_channel_param.param = obj;

		_lst_send_waits.push_back(cur_co);

		printf(
			"[%s] ccccccccccccccc, send_without_cache, suspend, tid:%d, cid:%d, _lst_send_waits.size:%ld\n", 
			date_ms().c_str(),
			gettid(),
			getcid(),
			_lst_send_waits.size()
		);

		if (_lst_recv_waits.size() > 0) {
			auto resume_co = _lst_recv_waits.front();
			_lst_recv_waits.pop_front();
			// for test
			assert(cur_co->_id != resume_co->_id);

			g_manager.resume_co(resume_co->_id);
		}

		if (is_in_co_env()) {
			g_schedule->switch_to_main([this] {
				_mutex.unlock();
			});
		} else {
			g_manager.add_suspend_co(cur_co);
			_mutex.unlock();
			_sem_send->wait();
		}

		if (_closed) {
			THROW_EXCEPTION("channel close");
		}
		if (is_in_co_env()) {
			if (cur_co->_channel_param.type != CHANNEL_BLOCK_NULL) {
				printf(
					"[%s] ccccccccccccccc, tid:%d, cid:%d, channel_param_type:%d\n", 
					date_ms().c_str(),
					gettid(),
					getcid(),
					cur_co->_channel_param.type
				);
			}
			assert(cur_co->_channel_param.type == CHANNEL_BLOCK_NULL);
		}
	}

	void recv_without_cache(T& obj) {

		_mutex.lock();

		if (_closed) {
			_mutex.unlock();
			THROW_EXCEPTION("channel close");
		}

		if (!_lst_send_waits.size()) {

			do {
				shared_ptr<Coroutine> cur_co;
				if (is_in_co_env()) {
					cur_co = g_manager.get_running_co();
				} else {
					cur_co = g_manager.get_free_co();
					cur_co->_func = [this] {
						_sem_recv->signal();
					};
				}

				assert(cur_co);
				cur_co->_state = SUSPEND;
				cur_co->_channel_param.type = CHANNEL_BLOCK_RECV;
				cur_co->_channel_param.param.Reset();

				_lst_recv_waits.push_back(cur_co);

				printf(
					"[%s] ccccccccccccccc, recv_without_cache, suspend because no send waits, tid:%d, cid:%d\n",
					date_ms().c_str(),
					gettid(),
					getcid()
				);

				if (is_in_co_env()) {
					g_schedule->switch_to_main([this] {
						_mutex.unlock();
					});
				} else {
					g_manager.add_suspend_co(cur_co);
					_mutex.unlock();
					_sem_recv->wait();
				}

				if (_closed) {
					THROW_EXCEPTION("channel close");
				}
				_mutex.lock();
				// 虽然被唤醒了，但是又可能被其他协程抢占了
				if (_lst_send_waits.size() > 0) {
					break ;
				}
			} while (1);
		}

		auto resume_co = _lst_send_waits.front();
		_lst_send_waits.pop_front();

		obj = resume_co->_channel_param.param.AnyCast<T>();
		resume_co->_channel_param.type = CHANNEL_BLOCK_NULL;
		resume_co->_channel_param.param.Reset();

		printf(
			"[%s] ccccccccccccccc, recv_without_cache, wake up and resume send waits, tid:%d, cid:%d, resume_cid:%d, channel_param_type:%d\n", 
			date_ms().c_str(),
			gettid(),
			getcid(),
			resume_co->_id,
			resume_co->_channel_param.type
		);

		g_manager.resume_co(resume_co->_id);
		_mutex.unlock();
	}

private:
	bool _closed;

	size_t _max_size;

	list<shared_ptr<Coroutine>> _lst_send_waits;	
	list<shared_ptr<Coroutine>> _lst_recv_waits;	

	list<T> _queue;

	CoMutex _mutex;

	semaphore*	_sem_send;
	semaphore*	_sem_recv;
};

#endif
