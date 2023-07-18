#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include <string>

using namespace std;

#define THROW_EXCEPTION(fmt, ...) \
{ \
	char ex[1024]; \
	snprintf(ex, sizeof(ex), "[FILE:%s,LINE:%d] exception:" fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
	throw string(ex); \
}

//#define now() time(NULL)

#define now_ms() \
({ \
	struct timeval tv; \
	gettimeofday(&tv, NULL); \
	(tv.tv_sec * 1000 + tv.tv_usec / 1000); \
})

template <class T>
class Singleton
{
public:
	static T* get_instance() {
		static T obj;
		return &obj;
	}
};

inline std::string date_ms(long time_ms = 0) {
    char date[32];
    time_t sec;
    long msec;
    if (time_ms > 0) {
        sec = time_ms / 1000;
        msec = time_ms % 1000;
    } else {
        struct timeval tvTime;
        gettimeofday(&tvTime, NULL);
        sec = tvTime.tv_sec;
        msec = tvTime.tv_usec / 1000;
    }

    struct tm tmTime;
    localtime_r(&sec, &tmTime);
    snprintf(
        date,
        sizeof(date),
        "%04d-%02d-%02d_%02d:%02d:%02d.%03ld",
        tmTime.tm_year + 1900,
        tmTime.tm_mon + 1,
        tmTime.tm_mday,
        tmTime.tm_hour,
        tmTime.tm_min,
        tmTime.tm_sec,
        msec
    );
    return date;
}

#endif
