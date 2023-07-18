#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include <string>

using namespace std;

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

template <class... Args>
string format_string(const string& fmt, Args... args)
{
    const int MAX_FORMAT_BUF_SIZE = 1024;
    string ret_msg;
    auto size = snprintf(nullptr, 0, fmt.c_str(), args...) + 1;
    if (size <= MAX_FORMAT_BUF_SIZE) {
        char msg[MAX_FORMAT_BUF_SIZE];
        snprintf(msg, sizeof(msg), fmt.c_str(), args...);
        ret_msg = msg;
    } else {
        unique_ptr<char[]> buf(new char[size]);
        snprintf(buf.get(), size, fmt.c_str(), args...);
        ret_msg = buf.get();
    }
    return ret_msg;
}

inline string date_ms(long time_ms = 0)
{
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
