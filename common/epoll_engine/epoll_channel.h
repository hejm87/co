#ifndef __EPOLL_CHANNEL_H__
#define __EPOLL_CHANNEL_H__

#include "../common/buffer.h"
#include "../common/net_utils.h"

//class ProtocolBase
//{
//public:
//    virtual size_t get_header_size() = 0;
//    virtual size_t get_packet_size(const char* data) = 0;
//    virtual bool parser(const char* data, size_t size) = 0;
//    virtual string to_buf() = 0;
//};

class EpollChannel : enable_shared_from_this<EpollEngine>
{
public:
    EpollChannel();
    virtual ~EpollChannel() = 0;

    bool init(int fd, shared_ptr<EpollEngine> engine, shared_ptr<void> argv = nullptr) {
        bool flag; 
        auto ret = NetUtils::get_socket_block_flag(fd, flag);
        if (ret != 0 || flag) {
            return false;
        }
        _engine = engine;
        return true;
    }

    virtual bool on_send() {;}
    virtual bool on_recv(const char* data, size_t size) {;}
    virtual void on_close() {;}
    virtual void on_error(int error) {;}
    virtual void on_timeout() {;}

    int fd() {
        return _fd;
    }

protected:
    int _fd;
    Buffer _w_buf;
    Buffer _r_buf;
    mutex  _mutex;
    shared_ptr<void> _argv;
    weak_ptr<EpollEngine> _engine;
};

template <class Protocol>
class EpollChannelProtocol : public EpollChannel
{
public:
    EpollChannelProtocol();
    virtual ~EpollChannelProtocol();

    bool on_send() {
        int ret;
        {
            lock_guard<mutex> lock(_mutex);
            auto send_size = _w_buf.used_size();
            if (send_size > 0) {
                return true;
            }
            ret = send(_fd, _w_buf.data(), send_size);
            if (ret > 0) {
                _w_buf.skip(ret);
                if (send_size == ret) {
                    auto e = _engine.lock();
                    if (!e) {
                        throw runtime_error("channel.on_send, EpollEngine is invalid");
                    }
                    e.del(shared_from_this(), EPOLL_WRITE);
                }
            }
        }
        if (ret > 0 || errno == EAGAIN || errno == EWOULDBLOCK) {
            return true;
        }
        return false;
    }

    bool on_recv(const char* data, size_t size) {
        Protocol protocol;
        {
            lock_guard<mutex> lock(_mutex);
            _r_buf.set(data, size);
            if (_r_buf.size() <= Protocol::get_header_size() || 
                _r_buf.size() < Protocol::get_packet_size(_r_buf.data())) {
                return true;
            }
            auto packet_size = Protocol::get_packet_size(_r_buf.data());
            if (!protocol.parser(_r_buf.data(), packet_size)) {
                return false;
            }
            _r_buf.skip(packet_size);
        }
        if (!on_message(protocol)) {
            return false;
        }
        return true;
    }

    virtual void on_close(int error) {
        ;
    }

    virtual void on_error(int error) {
        ;
    }

    virtual void on_timeout() {
        ;
    }

    bool send_packet(const Protocol& protocol) {
        auto buffer = protocol.to_buf();
        {
            lock_guard<mutex> lock(_mutex);
            _w_buf.set(buffer.c_str(), buffer.size());
        }
        _engine.set(this, EPOLL_SEND);
        return true;
    }

    virtual bool on_message(const Protocol& protocol) {
        return true;
    }
};

#endif