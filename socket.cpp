#include "socket.h"

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

int Socket::open(const char* host, int port)
{
    struct addrinfo* res = 0;
    struct addrinfo* res0 = 0;
    char port_str[sizeof("65535")];
    sprintf(port_str, "%d", port);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;

    int error = getaddrinfo(host, port_str, &hints, &res0);
    if (error) return -1; 

    for (res = res0; res; res = res->ai_next)
    {
        if (openConnection(res) == 0)
            break;
        else
        {
            if (res->ai_next)
            {
                close();
            }
            else
            {
                close();
                freeaddrinfo(res0);
                return -1;
            }
        }
    }

    freeaddrinfo(res0);
    return 0;
}

int Socket::openConnection(struct addrinfo* res)
{
    _fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (_fd == -1) return -1;

    struct linger l = {1, 0};
    setsockopt(_fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l));

    int v = 1;
    setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, &v, sizeof(v));

    int flags = fcntl(_fd, F_GETFL, 0);
    if (-1 == fcntl(_fd, F_SETFL, flags | O_NONBLOCK))
    {
        ::close(_fd);
        _fd = -1;
        return -2;
    }

    int ret = connect(_fd, res->ai_addr, static_cast<int>(res->ai_addrlen));
    if (ret == 0)
        return 0;

    if (errno != EINPROGRESS)
    {
        ::close(_fd);
        _fd = -1;
        return -3;
    }

    return 0;
}

