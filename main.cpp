#include <stdio.h>
#include <assert.h>
#include <string>
#include <thread>
#include <atomic>

#include <map>

#include "co_api.h"
#include "co.h"
#include "co_mutex.h"
#include "co_channel.h"
#include "co_semaphore.h"
#include "co_timer.h"
#include "utils.h"
#include "common/common.h"

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
void timer_test1();
void await_test();
void await_test1();

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
//	channel_cache_test1();
//	semaphore_test();
//	semaphore_test1();
//	sleep_test();
//	timer_test();
//	timer_test1();
//	await_test();
	await_test1();
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
				g_manager.yield();
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
		} catch (CoException& ex) {
			printf("ex:%s\n", ex.get_desc().c_str());
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
			} catch (CoException& ex) {
				printf("cid:%d, ex:%s\n", getcid(), ex.get_desc().c_str());
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
	} catch (CoException& ex) {
		printf("ex:%s\n", ex.get_desc().c_str());
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
		printf("[%s] ############### tid:%d, cid:%d, reader start\n", date_ms().c_str(), gettid(), getcid());
		g_manager.sleep_ms(SLEEP_MS);
		printf("[%s] ############### tid:%d, cid:%d, reader sleep finish\n", date_ms().c_str(), gettid(), getcid());
		do {
			try {
				int v;
				for (int i = 0; i < LOOP; i++) {
			//	for (int i = 0; i < 1; i++) {
					printf("[%s] ############### tid:%d, cid:%d, before read\n", date_ms().c_str(), gettid(), getcid());
					chan >> v;
					assert(v == i);
					printf("[%s] ############### tid:%d, cid:%d, read value:%d\n", date_ms().c_str(), gettid(), getcid(), v);
				}
			//	printf("[%s] ############### tid:%d, cid:%d, sleep before read\n", date_ms().c_str(), gettid(), getcid());
			//	g_manager.sleep_ms(SLEEP_MS);
				printf("[%s] ############### tid:%d, cid:%d, final read value\n", date_ms().c_str(), gettid(), getcid());
				chan >> v;
				printf("[%s] ############### tid:%d, cid:%d, read value:%d\n", date_ms().c_str(), gettid(), getcid(), v);
			} catch (CoException& ex) {
				printf("ex:%s\n", ex.get_desc().c_str());
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
			} catch (CoException& ex) {
				printf("[%s] ############### cid:%d, send, ex:%s\n", date_ms().c_str(), getcid(), ex.get_desc().c_str());
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
				} catch (CoException& ex) {
					printf("[%s] ############### cid:%d, recv, ex:%s\n", date_ms().c_str(), getcid(), ex.get_desc().c_str());
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

	printf("[%s] ###############, sem_value:%d\n", date_ms().c_str(), sem.get_value());

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
	const int LOOP = 50;
	const int SLEEP_MS = 10;

	atomic<bool> is_set(false);

	auto beg = now_ms();

	g_manager.create([&is_set, SLEEP_MS] {
		auto co_beg = now_ms();
		for (int i = 0; i < LOOP; i++) {
			g_manager.sleep_ms(SLEEP_MS);
		}
		auto co_end = now_ms();
		printf("[%s] ############### tid:%d, cid:%d, co_total_sleep:%ldms\n", date_ms().c_str(), gettid(), getcid(), co_end - co_beg);
		is_set = true;
	});

	while (!is_set) {
		;
	}

	auto end = now_ms();

	printf("cost:%ldms\n", end - beg);

	assert(end - beg >= LOOP * SLEEP_MS);
	assert(end - beg < LOOP * SLEEP_MS + (LOOP * SLEEP_MS) / 10);
}

void timer_test()
{
	const int SLEEP_MS = 100;

	auto id1 = CoApi::set_timer(SLEEP_MS, [] {
		CoApi::sleep_ms(SLEEP_MS);
	});
	auto id2 = CoApi::set_timer(SLEEP_MS, [] {
		CoApi::sleep_ms(SLEEP_MS);
	});

	assert(CoApi::cancel_timer(id2) == 0);

	usleep(10 * 1000);

//	printf("id1.state:%d\n", CoApi::get_timer_state(id1));
//	printf("id2.state:%d\n", CoApi::get_timer_state(id2));

	assert(CoApi::get_timer_state(id1) == CO_TIMER_WAIT);
	assert(CoApi::get_timer_state(id2) == CO_TIMER_CANCEL);

	usleep(100 * 1000);
	assert(CoApi::get_timer_state(id1) == CO_TIMER_PROCESS);

	usleep(200 * 1000);
	assert(CoApi::get_timer_state(id1) == CO_TIMER_FINISH);

	printf("all finish");
}

void timer_test1()
{
	const int TIMER_COUNT = 10;
	const int TIMER_PERIOD = 100;

	struct {
		long expect_time;
		long finish_time;
	} check_timer[TIMER_COUNT];

	atomic<int> end_count(0);

	for (int i = 0; i < TIMER_COUNT; i++) {
		auto delay_ms = (i + 1) * TIMER_PERIOD;
		check_timer[i].expect_time = now_ms() + delay_ms;
		CoApi::set_timer(delay_ms, [&end_count, &check_timer, i] {
			check_timer[i].finish_time = now_ms();
			end_count++;
		});
	}

	while (end_count != TIMER_COUNT) {
		usleep(100 * 1000);
	}

	for (int i = 0; i < TIMER_COUNT; i++) {
		auto& e = check_timer[i];
		printf(
			"################ i:%d, expect:%ld, finish:%ld, delta:%ld\n", 
			i, 
			e.expect_time, 
			e.finish_time, 
			e.finish_time - e.expect_time
		);
		assert(e.finish_time >= e.expect_time);
		assert(e.finish_time < e.expect_time + 10);
	}
	printf("all finish\n");
}

void await_test()
{
	const int CO_COUNT = 3;

	atomic<int> co_end_count(0);

	auto beg = now_ms();
	auto a = CoApi::create([] {
		CoApi::sleep_ms(100);
		return ;
	});

	for (int i = 0; i < CO_COUNT; i++) {
		CoApi::create([&a, &co_end_count, beg] {
			a.wait();
			auto end = now_ms();
			assert(end - beg >= 100);
			co_end_count++;
		});
	}

	a.wait();
	while (co_end_count != CO_COUNT) {
		usleep(10 * 1000);
	}

	auto end = now_ms();
	assert(end - beg >= 100);
	printf("all finish\n");
}

void await_test1()
{
	const int CO_COUNT = 3;

	atomic<int> co_end_count(0);

	auto beg = now_ms();
	auto a = CoApi::create([]() -> int {
		CoApi::sleep_ms(10);
		return 99;
	});

	for (int i = 0; i < CO_COUNT; i++) {
		CoApi::create([&a, &co_end_count, beg] {
			auto v = a.wait();
			printf("coroutine, wait, v:%d\n", v);
			auto end = now_ms();
			assert(v == 99);
			assert(end - beg >= 10);
			co_end_count++;
		});
	}

	auto v = a.wait();
	printf("main_thread, wait, v:%d\n", v);
	assert(v == 99);
	while (co_end_count != CO_COUNT) {
		usleep(1 * 1000);
	}

	auto end = now_ms();
	assert(end - beg >= 10);
	printf("all finish\n");
}
