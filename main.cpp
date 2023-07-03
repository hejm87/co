#include <stdio.h>
#include <assert.h>
#include <string>
#include <thread>
#include <atomic>
#include "co.h"
#include "co_mutex.h"
#include "co_channel.h"
#include "utils.h"

using namespace std;

const int CO_COUNT = 100;
const int THREAD_COUNT = 3;

atomic<int> g_count(0);

void reuse_test();
void exception_test();
void yield_test();
void lock_test();
void channel_test();

int main()
{
//	reuse_test();
//	exception_test();
//	yield_test();
//	lock_test();
	channel_test();
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
	int loop  = 100;
	int co_count = 2;

	CoMutex mutex;

	for (int id = 0; id < co_count; id++) {
		g_manager.create([&mutex, loop, id, &total] {
			for (int i = 0; i < loop; i++) {
				mutex.lock();
				printf("[%s] ############### tid:%d, cid:%d, app.lock succeed\n", date_ms().c_str(), gettid(), getcid());
				total++;
				mutex.unlock();
				printf("[%s] ############### tid:%d, cid:%d, app.unlock succeed\n", date_ms().c_str(), gettid(), getcid());
			}
		});
	}
	usleep(100 * 1000);
	printf("[%s] after wait, a1:%d, a2:%d\n", date_ms().c_str(), co_count * loop, total);
	assert(co_count * loop == total);
}

void channel_test()
{
	atomic<bool> is_end(false);
	CoChannel<int> chan;

	g_manager.create([&chan] {
		for (int i = 0; i < 10; i++) {
			printf("[%s] ############ tid:%d, cid:%d, ready to write value:%d\n", date_ms().c_str(), gettid(), getcid(), i);
			chan << i;
			printf("[%s] ############ tid:%d, cid:%d, write value:%d\n", date_ms().c_str(), gettid(), getcid(), i);
			usleep(200 * 1000);
		}
		chan.close();
		printf("[%s] ############ tid:%d, cid:%d, chan.close\n", date_ms().c_str(), gettid(), getcid());
	});

	g_manager.create([&chan, &is_end] {
		try {
			do {
				int v;
				printf("[%s] ############ tid:%d, cid:%d, ready to read\n", date_ms().c_str(), gettid(), getcid());
				chan >> v;
				printf("[%s] ############ tid:%d, cid:%d, read value:%d\n", date_ms().c_str(), gettid(), getcid(), v);
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
