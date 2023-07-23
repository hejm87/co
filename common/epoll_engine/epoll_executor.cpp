#include "epoll_engine.h"

const int RECV_BUF_SIZE = (1024 * 32);

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

EpollEngine::EpollEngine(int thread_count, int max_conn_count)
{
    _cur_conn_count = 0;
    _max_conn_count = max_conn_count;
    if (!create_epoll_infos()) {
        throw runtime_error("EpollEngine.create_epoll_infos fail");
    }
    for (int i = 0; i < _thread_count; i++) {
        _threads.emplace_back(thread([this, i] {
            run(i);
        }));
    }
}

int EpollEngine::set(inf fd, EpollEvent ev, size_t timeout, shared_ptr<void> argv)
{
    lock_guard<mutex> lock(_mutex);
    if (_cur_count >= _max_count) {
        return -1;
    }

    struct epoll_event ev;
    ev.events  = EPOLLIN;
    ev.data.fd = fd;

    auto epoll_id = _epoll_infos[fd % _thread.size()].epoll_id;
    auto ret = epoll_ctl(epoll_id, EPOLL_CTL_ADD, fd, &ev);
    if (ret == -1) {
        return -1;
    }

    auto& fd_argv = EPOLL_RECV ? _r_fd_argv : _w_fd_argv;
    fd_argv.insert(make_pair(fd, argv));
    _cur_count++;

    xxxx

    return 0;
}

int EpollEngine::del(int fd, EpollEvent ev)
{
    lock_guard<mutex> lock(_mutex);
    auto epoll_id = _epoll_infos[fd % _thread.size()].epoll_id;
    auto ret = epoll_ctl(epoll_id, EPOLL_CTL_DEL, fd, NULL);
    if (ret == -1) {
        return ret;
    }

    auto& fd_argv = EPOLL_RECV ? _r_fd_argv : _w_fd_argv;
    fd_argv.erase(fd);

    xxxx

}

void EpollEngine::terminate(bool wait)
{
    for (auto& item : _epoll_infos) {
        close(item.pipes[0]);
        close(item.pipes[1]);
    }
    for (auto& item : _threads) {
        wait ? item.join() : item.detach();
    }
}

bool EpollEngine::create_epoll_info(EpollInfo& info)
{
    memset(&info, 0, sizeof(info));
    bool flag = false;
    do {
        info.epoll_id = epoll_create(_max_count);
        if (info.epoll_id == -1) {
            break ;
        }
        if (pipe(info.pipes) == -1) {
            break ;
        }
        info.events = malloc(_max_count * sizeof(struct epoll_event));
        if (info.events) {
            break ;
        }
        flag = true;
    } while (0);

    if (!flag) {
        if (info.epoll_id > 0) {
            close(info.epoll_id);
        }
        if (info.pipes[0] > 0) {
            close(info.pipes[0]);
        }
        if (info.pipes[1] > 0) {
            close(info.pipes[1]);
        }
    }
    return flag; 
}

bool EpollEngine::create_epoll_infos()
{
    int i = 0;
    for (; i < _thread_count; i++) {
        EpollInfo info;
        if (!create_epoll_info(info)) {
            break ;
        }
        _epoll_infos.push_back(info);
    }
    if (i != _thread_count) {
        for (auto& item : _epoll_infos) {
            close(item.epoll_id);
            close(item.pipes[0]);
            close(item.pipes[1]);
        }
        _epoll_infos.clear();
        return false;
    }
    return true;
}

//int epoll_wait(
//    int epfd, 
//    struct epoll_event *events, 
//    int maxevents, 
//    int timeout
//);

void EpollEngine::run(int index)
{
    EpollInfo& info = _epoll_infos[index];
    while (1) {
        auto count = epoll_wait(info.epoll_id, info.events, _max_count, -1);
        if (count == -1 && errno != INTR) {
            runtime_error("epoll_wait fail");
        }
        for (int i = 0; i < count; i++) {
            auto chan = get_channel(info.events[i].fd);
            if (!chan) {
                continue ;
            }
            const auto& ev = info.events[i];
            if (ev.events & EPOLLIN) {
                char recvbuf[RECV_BUF_SIZE];
                auto ret = recv(info.ev, recvbuf, sizeof(recvbuv));
                if (ret > 0) {
                    chan->on_recv(recvbuf, ret);
                } else if (ret == 0) {
                    chan->on_close();
                } else {
                    chan->on_error(errno);
                }
            } else if (ev.events & EPOLLOUT) {
                chan->on_send(); 
            }
        }

    }
}