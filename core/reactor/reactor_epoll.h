#ifndef __REACTOR_EPOLL_H__
#define __REACTOR_EPOLL_H__

#include "reactor.h"

#include <sys/epoll.h>

namespace evnet
{

class ReactorEpoll : public Reactor
{
    public:
        ReactorEpoll();
        virtual ~ReactorEpoll();

        int init();

    public:
        static void epoll_modify(EvLoop* loop, int fd, int old_flag, int new_flag);
        static void epoll_poll(EvLoop* loop, int timeout);
        static void epoll_wakeup(EvLoop* loop);
        static int epoll_init(EvLoop* loop, int flags);
        static int epoll_destroy(EvLoop* loop);

    private:
        int _epoll_fd;

        int _pipe_read;
        int _pipe_write;

        int _event_max;
        struct epoll_event* _events;
};

}

#endif
