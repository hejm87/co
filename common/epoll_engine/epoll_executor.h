#ifndef __EPOLL_EXECUTOR_H__
#define __EPOLL_EXECUTOR_H__

#include <sys/epoll.h>

#include <mutex>
#include <thread>
#include <memory>
#include <functional>

#include <map>
#include <vector>

#include "epoll_channel.h"

using namespace std;

enum EpollEvent
{
    EPOLL_RECV = 0x1,
    EPOLL_SEND = 0x2,
};

struct EpollInfo
{
    int epoll_id;
    int pipes[2];
    struct epoll_event* events;
};

class EpollChannel;

class EpollEngine : enable_shared_from_this<EpollEngine>
{
public:
    EpollEngine(int thread_count, int max_conn_count);
    ~EpollEngine();

    int set(int fd, EpollEvent ev, size_t timeout, shared_ptr<void> argv = nullptr);
    int del(int fd, EpollEvent ev);

    void terminate(bool wait = true);

protected:
    virtual void on_send(int fd, shared_ptr<void> argv) {;}
    virtual void on_recv(int fd, shared_ptr<void> argv, char* data, size_t size) {;}
    virtual void on_close(int fd, shared_ptr<void> argv) {;}
    virtual void on_error(int fd, shared_ptr<void> argv, int error) {;}

private:
    bool create_epoll_info(EpollInfo& info);
    bool create_epoll_infos();
    void run(int index);

    shared_ptr<EpollChannel> get_channel(int fd);

private:
    int _max_count;
    int _cur_count;

    map<int, shared_ptr<void>> _r_fd_argv;
    map<int, shared_ptr<void>> _w_fd_argv;

    multimap<int, int>  _lst_timer;

    mutex   _mutex;
    vector<thread>      _threads;
    vector<EpollInfo>   _epoll_infos;
};

#endif
