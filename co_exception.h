#ifndef __CO_EXCEPTION_H__
#define __CO_EXCEPTION_H__

#include <stdarg.h>

#include <utility>
#include <exception>

#include "utils.h"

using namespace std;

class CoException;

#define THROW_EXCEPTION(code, fmt, ...) \
{ \
    throw CoException(code, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
}

const int CO_ERROR_UNKNOW = -1;
const int CO_ERROR_PARAM_INVALID = 1;         // 非法参数
const int CO_ERROR_CHANNEL_CLOSE = 2;         // channel已关�?
const int CO_ERROR_UNLOCK_ILLEGAL = 3;        // 非锁状态下unlock
const int CO_ERROR_NOT_IN_CO_ENV  = 4;        // 非协程环境下进�?�操�?

class CoException : public exception
{
public:
    CoException(int code, const string& file, int line, const string& msg) {
        _code = code;
        _file = file;
        _line = line;
        _msg  = msg;
    }

    template <class ...Args>
    CoException(int code, const string& file, int line, const string& fmt, Args... args) {
        _code = code;
        _file = file;
        _line = line;
        _msg  = format_string(fmt, forward<Args>(args)...);
    }

    int     get_code() {return _code;}
    string  get_file() {return _file;}
    int     get_line() {return _line;}
    string  get_msg()  {return _msg;}

    string get_desc() {
        return format_string(
            "[file:%s, line:%d] code:%d, errmsg:%s", 
            _file.c_str(), 
            _line, 
            _code, 
            _msg.c_str()
        );
    }

private:
    int     _code;
    string  _file;
    int     _line;
    string  _msg;
};

#endif