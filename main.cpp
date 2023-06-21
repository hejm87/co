#include <stdio.h>
#include <assert.h>
#include <string>
#include <thread>
#include <atomic>
#include "co.h"

using namespace std;

const int CO_COUNT = 100;
const int THREAD_COUNT = 3;

atomic<int> g_count(0);

void reuse_test();
void exception_test();
void yield_test();

int main()
{
	reuse_test();
//	exception_test();
//	yield_test();
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

