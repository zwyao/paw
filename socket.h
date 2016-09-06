#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>

struct addrinfo;
class Socket
{
    public:
        Socket():
            _fd(-1)
        {
        }

        ~Socket()
        {
            close();
        }

        int open(const char* host, int port);

        void close()
        {
            if (_fd != -1)
            {
                shutdown(_fd, SHUT_RDWR);
                ::close(_fd);
                _fd = -1;
            }
        }

        int fd() const { return _fd; }

    private:
        int openConnection(struct addrinfo* res);

    private:
        int _fd;
};

#endif

