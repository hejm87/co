#ifndef __NET_UTILS_H__
#define __NET_UTILS_H__

class NetUtils
{
public:
    static int set_socket_block(int fd);
    static int set_socket_unblock(int fd);

    static int get_socket_block_flag(int fd, bool& block);

};

#endif