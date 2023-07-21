#ifndef __EPOLL_ENGINE_H__
#define __EPOLL_ENGINE_H__

#include <sys/epoll.h>

#include <memory>
#include <functional>

enum EpollEvent
{
    EPOLL_READ  = 0x1,
    EPOLL_WRITE = 0x2,
};

class EpollChannel;

class EpollEngine
{
public:
    EpollEngine(int thread_count, int max_conn_count);
    ~EpollEngine();

    int set(shared_ptr<EpollChannel> chan, EpollEvent ev);
    int del(shared_ptr<EpollChannel> chan, EpollEvent ev);

private:
    void thread_run();

private:
    atomic<bool> _is_set_end;

    int _thread_count;
    int _max_conn_count;

    map<int, shared_ptr<EpollChannel>> _r_fd_chan;
    map<int, shared_ptr<EpollChannel>> _w_fd_chan;

    mutex _mutex;
};

#endif