#ifndef __EPOLL_EXECUTOR_H__
#define __EPOLL_EXECUTOR_H__

#include <sys/epoll.h>

#include <memory>
#include <functional>

enum EpollEvent
{
    EPOLL_READ  = 0x1,
    EPOLL_WRITE = 0x2,
};

class EpollChannel;

class EpollExecutor
{
public:
    EpollExecutor(int count);
    ~EpollExecutor();

    int set(shared_ptr<EpollChannel> chan, EpollEvent ev, size_t timeout);
    int del(shared_ptr<EpollChannel> chan, EpollEvent ev);

    void set_end() {
        close(_pipes[0]);
    }

private:
    void run();

private:
    int _epoll_id;
    int _max_count;
    int _cur_count;
    int _pipes[2];

    map<int, shared_ptr<EpollChannel>> _r_fd_chan;
    map<int, shared_ptr<EpollChannel>> _w_fd_chan;

    mutex   _mutex;
    thread  _thread;
};

#endif