#include <stdio.h>
#include <assert.h>
#include <string>
#include <thread>
#include <atomic>

#include <map>

#include "co.h"
#include "co_mutex.h"
#include "co_channel.h"
#include "co_semaphore.h"
#include "co_timer.h"
#include "utils.h"

using namespace std;

const int CO_COUNT = 100;
const int THREAD_COUNT = 3;

atomic<int> g_count(0);

void reuse_test();
void exception_test();
void yield_test();
void lock_test();
void lock_test1();
void channel_test();
void channel_test1();
void channel_test2();
void channel_cache_test();
void channel_cache_test1();
void semaphore_test();
void semaphore_test1();
void sleep_test();
void timer_test();

int main()
{
//	reuse_test();
//	exception_test();
//	yield_test();
//	lock_test();
//	lock_test1();
//	channel_test();
//	channel_test1();
//	channel_test2();
//	channel_cache_test();
	channel_cache_test1();
//	semaphore_test();
//	semaphore_test1();
//	sleep_test();
//	timer_test();
	return 0;
}

void reuse_test()
{
	int i = 0;
	while (i < 10) {
		auto ret = g_manager.create([i] {
			printf("tid:%d, cid:%d, i:%d\n", gettid(), getcid(), i);
		});
		if (ret) {
			i++;
		} else {
			printf("no coroutine to use, sleep 100ms\n");
			usleep(10 * 1000);
		}
	}
	usleep(100 * 1000);
	printf("reuse_test finish\n");
}

void exception_test()
{
	for (int i = 0; i < CO_COUNT; i++) {
		g_manager.create([i] {
			try {
				throw to_string(i);
			} catch (string& ex) {
				printf("tid:%d, cid:%d, value:%s\n", gettid(), getcid(), ex.c_str());
			}
			g_count++;	
		});
	}

	usleep(200 * 1000);
	printf("g_count:%d, CO_COUNT:%d\n", g_count.load(), CO_COUNT);
	assert(g_count == CO_COUNT);
	printf("main finish\n");
}

void yield_test()
{
	int co_count = 100;

	for (int i = 0; i < co_count; i++) {
		g_manager.create([i] {
			for (int j = 0; j < 3; j++) {
				printf("############# tid:%d, cid:%d, value:%d\n", gettid(), getcid(), j);
				g_schedule->yield();
			}
			printf("############# tid:%d, cid:%d, exit\n", gettid(), getcid());
			g_count++;
		});
	}

	while (g_count != co_count) {
		printf("########## g_count:%d, co_count:%d\n", g_count.load(), co_count);
		usleep(100 * 1000);
	}
	printf("skip while\n");

	usleep(100 * 1000);

	printf("main finish\n");
}

void lock_test()
{
	int total = 0;
	int loop  = 2000;
	int co_count = 2;
	atomic<int> end_count(0);

	CoMutex mutex;

	for (int id = 0; id < co_count; id++) {
		g_manager.create([&mutex, &end_count, loop, id, &total] {
			for (int i = 0; i < loop; i++) {
				int now_total;
				mutex.lock();
				printf("[%s] ############### tid:%d, cid:%d, app.lock succeed\n", date_ms().c_str(), gettid(), getcid());
				now_total = ++total;
				mutex.unlock();
				printf("[%s] ############### tid:%d, cid:%d, app.unlock succeed, total:%d\n", date_ms().c_str(), gettid(), getcid(), now_total);
			}
			end_count++;
		});
	}

	while (end_count != co_count) {
		usleep(10 * 1000);
	}

	printf("[%s] after wait, a1:%d, a2:%d\n", date_ms().c_str(), co_count * loop, total);
	assert(co_count * loop == total);
}

//void lock_test1()
//{
//	CoMutex mutex;
//
//	mutex.lock();
//
//	g_manager.create([&mutex] {
//		mutex.lock();
//		printf("[%s] ############### tid:%d, cid:%d, lock\n", date_ms().c_str(), gettid(), getcid());
//	});
//
//	g_manager.create([&mutex] {
//		usleep(200 * 1000);
//		printf("[%s] ############### tid:%d, cid:%d, ready to unlock\n", date_ms().c_str(), gettid(), getcid());
//		mutex.unlock();
//		printf("[%s] ############### tid:%d, cid:%d, unlock\n", date_ms().c_str(), gettid(), getcid());
//	});
//
//	sleep(1);
//	printf("[%s] ############### all finish\n", date_ms().c_str());
//}

void lock_test1()
{
	const int LOOP = 1000;

	map<int, int> mtotal;
	atomic<bool> is_co_end(false);

	CoMutex mutex;
	
	g_manager.create([&mutex, &mtotal, &is_co_end] {
		for (int i = 0; i < LOOP; i++) {
		//	printf("[%s] ###############, tid:%d, cid:%d, before lock\n", date_ms().c_str(), gettid(), getcid());
			mutex.lock();
		//	printf("[%s] ###############, tid:%d, cid:%d, after lock\n", date_ms().c_str(), gettid(), getcid());
			mtotal[0]++;
			mutex.unlock();
		//	printf("[%s] ###############, tid:%d, cid:%d, after unlock\n", date_ms().c_str(), gettid(), getcid());
		}
		is_co_end = true;
		printf("[%s] ###############, tid:%d, cid:%d, co finish\n", date_ms().c_str(), gettid(), getcid());
	});

	for (int i = 0; i < LOOP; i++) {
	//	printf("[%s] ###############, tid:%d, before lock\n", date_ms().c_str(), gettid());
		mutex.lock();
	//	printf("[%s] ###############, tid:%d, after lock\n", date_ms().c_str(), gettid());
		mtotal[1]++;
		mutex.unlock();
	//	printf("[%s] ###############, tid:%d, after unlock\n", date_ms().c_str(), gettid());
	}
	printf("[%s] ###############, tid:%d, main thread finish\n", date_ms().c_str(), gettid());

	while (!is_co_end) {
		usleep(10 * 1000);
	}

	printf("v1:%d, v2:%d, mtotal0:%d, mtotal1:%d\n", LOOP * 2, mtotal[0] + mtotal[1], mtotal[0], mtotal[1]);
	assert(mtotal[0] > 0);
	assert(mtotal[1] > 0);
	assert(LOOP * 2 == mtotal[0] + mtotal[1]);
}

void channel_test()
{
	atomic<bool> is_end(false);
	CoChannel<int> chan;

	g_manager.create([&chan] {
		for (int i = 0; i < 100; i++) {
			printf("[%s] ############ tid:%d, cid:%d, ready to write value:%d\n", date_ms().c_str(), gettid(), getcid(), i);
			chan << i;
			printf("[%s] ############ tid:%d, cid:%d, write value:%d\n", date_ms().c_str(), gettid(), getcid(), i);
		//	usleep(100 * 1000);
		}
		chan.close();
		printf("[%s] ############ tid:%d, cid:%d, chan.close\n", date_ms().c_str(), gettid(), getcid());
	});

	g_manager.create([&chan, &is_end] {
		int expect = 0;
		try {
			do {
				int v;
				printf("[%s] ############ tid:%d, cid:%d, ready to read\n", date_ms().c_str(), gettid(), getcid());
				chan >> v;
				printf("[%s] ############ tid:%d, cid:%d, read value:%d\n", date_ms().c_str(), gettid(), getcid(), v);
				assert(expect++ == v);
			} while (1);
		} catch (string& ex) {
			printf("ex:%s\n", ex.c_str());
		}
		is_end = true;
		printf("[%s] ############ tid:%d, cid:%d, is end\n", date_ms().c_str(), gettid(), getcid());
	});

	while (!is_end) {
		usleep(100 * 1000);
	}
	printf("[%s] ############ all finish\n", date_ms().c_str());
}

void channel_test1()
{
	const int LOOP = 10000;
	const int RECV_CO_COUNT = 2;

	CoMutex mutex;
	CoChannel<int> chan;
	atomic<int> end_count(0);
	map<int, int> co_count;

	g_manager.create([&chan] {
		for (int i = 0; i < LOOP; i++) {
			chan << i;
		//	printf("[%s] ############ write channel, i:%d\n", date_ms().c_str(), i);
		}
	//	printf("[%s] ############ write channel close\n", date_ms().c_str());
		chan.close();
	});

	for (int i = 0; i < RECV_CO_COUNT; i++) {
		g_manager.create([&chan, &mutex, &co_count, &end_count] {
			printf("[%s] ############ cid:%d, running\n", date_ms().c_str(), getcid());
			try {
				do {
					int v;	
					chan >> v;
				//	printf("[%s] ############ cid:%d, read channel succeed\n", date_ms().c_str(), getcid());
					mutex.lock();	
					co_count[getcid()]++;
					mutex.unlock();	
				//	printf("[%s] ############ cid:%d, read channel succeed, after\n", date_ms().c_str(), getcid());
				} while (1);
			} catch (string& ex) {
				printf("cid:%d, ex:%s\n", getcid(), ex.c_str());
			}
			end_count++;
		});
	}

	while (end_count != RECV_CO_COUNT) {
		usleep(10 *1000);
	}
	printf("co_count:%ld\n", co_count.size());
	assert(co_count.size() == 2);

	int total_count = 0;
	for (auto& item : co_count) {
		total_count += item.second;
	}
	printf("total_count:%d\n", total_count);
	assert(total_count == LOOP);

	printf("%s finish\n", __FUNCTION__);
}

void channel_test2()
{
	const int LOOP = 1000;

	int recv_count = 0;

	CoChannel<int> chan;

	g_manager.create([&chan] {
		for (int i = 0; i < LOOP; i++) {
			printf("[%s] ############### tid:%d, cid:%d, before, write %d\n", date_ms().c_str(), gettid(), getcid(), i);
			chan << i;
			printf("[%s] ############### tid:%d, cid:%d, after, write %d\n", date_ms().c_str(), gettid(), getcid(), i);
		}
		chan.close();
	});

	try {
		do {
			int v;
			printf("[%s] ############### tid:%d, cid:%d, before read\n", date_ms().c_str(), gettid(), getcid());
			chan >> v;
			printf("[%s] ############### tid:%d, cid:%d, after read, value:%d\n", date_ms().c_str(), gettid(), getcid(), v);
			recv_count++;
		} while (1);
	} catch (string& ex) {
		printf("ex:%s\n", ex.c_str());
	}

	printf("recv_count:%d\n", recv_count);
	assert(recv_count == LOOP);
}

void channel_cache_test()
{
	const int LOOP = 5;
	const int CACHE_SIZE = 5;
	const int SLEEP_MS = 100;

	atomic<bool> start_read(false);
	atomic<int>  end_count(0);

	CoChannel<int> chan(CACHE_SIZE);

	g_manager.create([&chan, &start_read, &end_count] {
		for (int i = 0; i < LOOP; i++) {
			printf("[%s] ############### tid:%d, cid:%d, before, write\n", date_ms().c_str(), gettid(), getcid());
			chan << i;
			printf("[%s] ############### tid:%d, cid:%d, after, write\n", date_ms().c_str(), gettid(), getcid());
		}
		auto beg_ms = now_ms();
		start_read = true;
		chan << 99;
		auto end_ms = now_ms();
		printf("[%s] ############### wait_ms:%ld, beg_ms:%ld, end_ms:%ld\n", date_ms().c_str(), end_ms - beg_ms, beg_ms, end_ms);
		assert(end_ms - beg_ms >= SLEEP_MS);
		chan.close();
		end_count++;
	});

	while (!start_read) {
		usleep(1);
	}

	g_manager.create([&chan, &end_count] {
		g_manager.sleep_ms(SLEEP_MS);
		do {
			try {
				int v;
				for (int i = 0; i < LOOP; i++) {
			//	for (int i = 0; i < 1; i++) {
					chan >> v;
					assert(v == i);
					printf("[%s] ############### tid:%d, cid:%d, read value:%d\n", date_ms().c_str(), gettid(), getcid(), v);
				}
			//	printf("[%s] ############### tid:%d, cid:%d, sleep before read\n", date_ms().c_str(), gettid(), getcid());
			//	g_manager.sleep_ms(SLEEP_MS);
				printf("[%s] ############### tid:%d, cid:%d, final read value\n", date_ms().c_str(), gettid(), getcid());
				chan >> v;
				printf("[%s] ############### tid:%d, cid:%d, read value:%d\n", date_ms().c_str(), gettid(), getcid(), v);
			} catch (string& ex) {
				printf("ex:%s\n", ex.c_str());
				break ;
			}
		} while (1);
		end_count++;
	});

	while (end_count != 2) {
		usleep(10 * 1000);
	}
	printf("all finish\n");
}

void channel_cache_test1()
{
	const int LOOP = 100;
	const int CACHE_SIZE = 5;
	const int SLEEP_MS = 100;

	const int SEND_COUNT = 2;
	const int RECV_COUNT = 3;

	atomic<int> send_total(0);
	atomic<int> recv_total(0);

	atomic<int> send_end_count(0);
	atomic<int> recv_end_count(0);

	CoChannel<int> chan(CACHE_SIZE);

	int beg_value[2] = {0, 1000};
	for (int i = 0; i < SEND_COUNT; i++) {
		auto index = i;
		g_manager.create([&chan, &send_total, &send_end_count, beg_value, index] {
			printf("[%s] ############### cid:%d, send beg\n", date_ms().c_str(), getcid());
			try {
				for (int i = 0; i < LOOP; i++) {
					auto score = beg_value[index] + i;
					printf("[%s] ############### cid:%d, send, before value:%d\n", date_ms().c_str(), getcid(), score);
					chan << score;
					printf("[%s] ############### cid:%d, send, after value:%d\n", date_ms().c_str(), getcid(), score);
					send_total += score;
				}
			} catch (string& ex) {
				printf("[%s] ############### cid:%d, send, ex:%s\n", date_ms().c_str(), getcid(), ex.c_str());
			}
			send_end_count++;
			printf("[%s] ############### cid:%d, send end\n", date_ms().c_str(), getcid());
		});
	}

	for (int i = 0; i < RECV_COUNT; i++) {
		g_manager.create([&chan, &recv_total, &recv_end_count] {
			printf("[%s] ############### cid:%d, recv beg\n", date_ms().c_str(), getcid());
			int v;
			do {
				try {
					chan >> v;
					printf("[%s] ############### cid:%d, recv, after value:%d\n", date_ms().c_str(), getcid(), v);
					recv_total += v;
				} catch(string& ex) {
					printf("[%s] ############### cid:%d, recv, ex:%s\n", date_ms().c_str(), getcid(), ex.c_str());
					break ;
				}
			} while (1);
			recv_end_count++;
			printf("[%s] ############### cid:%d, recv end\n", date_ms().c_str(), getcid());
		});
	}

	while (send_end_count != SEND_COUNT) {
		printf(
			"[%s] ############### send_end_count:%d, send_total:%d, recv_total:%d\n", 
			date_ms().c_str(), 
			send_end_count.load(),
			send_total.load(), 
			recv_total.load()
		);
		usleep(100 * 1000);
	}
	printf("[%s] ############### send coroutines end\n", date_ms().c_str());
	chan.close();

	while (recv_end_count != RECV_COUNT) {
		printf("[%s] ############### recv_end_count:%d\n", date_ms().c_str(), recv_end_count.load());
		usleep(100 * 1000);
	}
	printf("[%s] ############### recv coroutines end\n", date_ms().c_str());

	printf("[%s] ############### send_total:%d, recv_total:%d\n", date_ms().c_str(), send_total.load(), recv_total.load());
	assert(send_total == recv_total);
}

void semaphore_test()
{
	const int LOOP = 1000;

	long beg_ms, end_ms;
	
	atomic<int>  count(0);
	atomic<int>  end_count(0);
	atomic<bool> set_end(false);
	CoSemaphore sem(0);

	printf("sem_value:%d\n", sem.get_value());

	beg_ms = now_ms();

	g_manager.create([&sem] {
		for (int i = 0; i < LOOP; i++) {
		//	printf("[%s] ############### tid:%d, cid:%d, before signal\n", date_ms().c_str(), gettid(), getcid());
			sem.signal();
		//	printf("[%s] ############### tid:%d, cid:%d, after signal\n", date_ms().c_str(), gettid(), getcid());
			usleep(2 * 1000);
		}
	});

	for (int i = 0; i < 2; i++) {
		g_manager.create([&sem, &count, &set_end, &end_count, i] {
			do {
			//	printf("[%s] ############### tid:%d, cid:%d, before wait\n", date_ms().c_str(), gettid(), getcid());
				sem.wait();
			//	printf("[%s] ############### tid:%d, cid:%d, after wait\n", date_ms().c_str(), gettid(), getcid());
				if (set_end) {
					break ;
				}
				count++;
			} while (1);
			printf("[%s] ############### tid:%d, cid:%d, end\n", date_ms().c_str(), gettid(), getcid());
			end_count++;
		});
	}

	while (count < LOOP) {
		usleep(10 * 1000);
	}

	set_end = true;
	sem.signal();
	sem.signal();

	while (end_count < 2) {
		usleep(10 * 1000);
	}

	end_ms = now_ms();

	printf("all finish, count:%d, cost:%ldms\n", count.load(), end_ms - beg_ms);
	assert(count == LOOP);
}

void semaphore_test1()
{
	const int LOOP = 1000;

	long beg_ms, end_ms;

	atomic<int> send_count(LOOP);
	atomic<int> recv_count(0);
	atomic<int> end_count(0);
	atomic<bool> set_end(false);

	CoSemaphore sem;

	beg_ms = now_ms();

	// coroutine writer
	g_manager.create([&sem, &send_count, &end_count, &set_end] {
		while (--send_count >= 0) {
			sem.signal();
			usleep(200);
		}
	});

	// thread writer
	thread([&sem, &send_count, &end_count, &set_end] {
		while (--send_count >= 0) {
			sem.signal();
			usleep(200);
		}
	}).detach();

	// coroutine reader
	g_manager.create([&sem, &recv_count, &end_count, &set_end] {
		do {
			sem.wait();
			if (set_end) {
				break ;
			}
			recv_count++;
		} while (1);
		end_count++;
	});

	// thread reader
	thread([&sem, &recv_count, &end_count, &set_end] {
		do {
			sem.wait();
			if (set_end) {
				break ;
			}
			recv_count++;
		} while (1);	
		end_count++;
	}).detach();

	while (recv_count < LOOP) {
		printf("[%s] ############### count:%d, LOOP:%d\n", date_ms().c_str(), recv_count.load(), LOOP);
		usleep(100 * 1000);
	}
	printf("[%s] ############### LOOP is finish\n", date_ms().c_str());
	set_end = true;
	sem.signal();
	sem.signal();

	while (end_count < 2) {
		usleep(1);
	}
//	printf("[%s] ############### end gogogo\n", date_ms().c_str());
	end_ms = now_ms();
	printf("all finish, count:%d, cost:%ldms\n", recv_count.load(), end_ms - beg_ms);
	assert(recv_count == LOOP);
}

void sleep_test()
{
	const int SLEEP_MS = 500;

	atomic<bool> is_set(false);

	auto beg = now_ms();

	g_manager.create([&is_set, SLEEP_MS] {
		printf("[%s] ############ tid:%d, cid:%d, before sleep\n", date_ms().c_str(), gettid(), getcid());
		g_manager.sleep_ms(SLEEP_MS);
		printf("[%s] ############ tid:%d, cid:%d, after sleep\n", date_ms().c_str(), gettid(), getcid());
		is_set = true;
	});

	while (!is_set) {
		;
	}

	auto end = now_ms();

	printf("cost:%ldms\n", end - beg);

	assert(end - beg >= 500);
	assert(end - beg < 550);
}

void timer_test()
{
	const int DELAY_MS = 500;

	atomic<bool> is_set(false);

	auto beg = now_ms();

	printf("[%s] ############ ready to set timer, beg:%ld\n", date_ms().c_str(), beg);
	Singleton<CoTimer>::get_instance()->set(DELAY_MS, [&is_set] {
		printf("[%s] ############ tid:%d, cid:%d, timer trigger\n", date_ms().c_str(), gettid(), getcid());
		is_set = true;
	});

	while (!is_set) {
		;
	}

	auto end = now_ms();

	printf("cost:%ldms\n", end - beg);

	assert(end - beg >= 500);
	assert(end - beg < 550);
}
