#ifndef __EPOLL_CHANNEL_H__
#define __EPOLL_CHANNEL_H__

#include "../common/buffer.h"
#include "../common/net_utils.h"

class EpollChannel
{
public:
    EpollChannel();
    virtual ~EpollChannel() = 0;

    bool init(int fd, shared_ptr<EpollEngine> engine) {
        bool flag; 
        auto ret = NetUtils::get_socket_block_flag(fd, flag);
        if (ret != 0 || flag) {
            return false;
        }
        _engine = engine;
        return true;
    }

    virtual void on_send() {;}
    virtual void on_recv() {;}
    virtual void on_close() {;}
    virtual void on_error(int error) {;}

    int get_fd() {
        return _fd;
    }

private:
    int _fd;
    Buffer _w_buf;
    Buffer _r_buf;
    weak_ptr<EpollEngine> _engine;
};

template <class Protocol>
class EpollChannelProtocol : public EpollChannel
{
public:
    EpollChannelProtocol();
    virtual ~EpollChannelProtocol();

    void on_send() {
        ;
    }

    void on_recv() {
        ;
    }

    void on_close() {
        ;
    }

    void on_error(int error) {
        ;
    }
};

#endif