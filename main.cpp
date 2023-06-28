#include <stdio.h>
#include <assert.h>
#include <string>
#include <thread>
#include <atomic>
#include "co.h"

using namespace std;

atomic<int> g_count(0);

void schedule_test()
{
	const int CO_COUNT = 100;
	const int THREAD_COUNT = 3;

	vector<shared_ptr<CoSchedule>> co_schedules;

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

int main()
{
    schedule_test();
    return 0;
}
