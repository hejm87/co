#include "epoll_engine.h"

EpollEngine::EpollEngine(int thread_count, int max_conn_count)
{
    _is_set_end.store(false);
    _thread_count = thread_count;
    _max_conn_count = max_conn_count;
    _cur_conn_count = 0;
    for (int i = 0; i < _thread_count; i++) {
        auto epoll_id = epoll_create(_max_conn_count);
        if (epoll_id != 0) {
            throw runtime_error("epoll create fail");
        }
        _epoll_ids.push_back(epoll_id);
        _threads.emplace_back([this, epoll_id] {
            thread_run(epoll_id);
        });
    }
}

int EpollEngine::set(shared_ptr<EpollChannel> chan, EpollEvent ev)

int EpollEngine::del(shared_ptr<EpollChannel> chan, EpollEvent ev)

void EpollEngine::thread_run(int epoll_id)
{

}