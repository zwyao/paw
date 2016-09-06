#include "ev_process_internal.h"
#include "reactor_epoll.h"

#include <new>

namespace evnet
{

namespace
{

inline void ev_dummy_cb(void* obj, int event) {}

EvCore _dummy = {0, 0, 0, ev_dummy_cb};

inline ReactorEpoll* create_epoll_reactor()
{
    ReactorEpoll* t = new (std::nothrow) ReactorEpoll();
    if (t == 0)
        abort();
    if (t->init() != 0)
    {
        delete t;
        return 0;
    }

    return t;
}

inline EvLoop* create_ev_loop()
{
    EvLoop* loop = (EvLoop*)ev_malloc(sizeof(EvLoop));
    memset(loop, 0, sizeof(EvLoop));
    return loop;
}

}

EvLoop* ev_init(unsigned int flags)
{
    Reactor* reactor = 0;

    if (IS_REACTOR_EPOLL(flags))
    {
        reactor = create_epoll_reactor();
        if (reactor == 0)
        {
            return 0;
        }
    }
    else if (IS_REACTOR_SELECT(flags))
    {
        return 0;
    }
    else if (IS_REACTOR_POOL(flags))
    {
        return 0;
    }
    else
    {
        reactor = create_epoll_reactor();
        if (reactor == 0)
        {
            return 0;
        }
    }

    EvLoop* loop = create_ev_loop();
    loop->reactor = reactor;
    loop->ev_dummy = &_dummy;
    loop->now = ev_time();

    return loop;
}

void ev_destroy(EvLoop* loop)
{
    int i = 0;
    while (i < NUMPRI)
    {
        if (loop->pendings[i])
            ev_free(loop->pendings[i]);
        ++i;
    }

    if (loop->fd_changes)
        ev_free(loop->fd_changes);

    if (loop->fds)
        ev_free(loop->fds);

    if (loop->fd_error)
        ev_free(loop->fd_error);

    if (loop->timers)
        ev_free(loop->timers);

    if (loop->reactor)
        delete loop->reactor;

    delete loop;
}

void ev_loop(EvLoop* loop)
{
    if (unlikely(loop == 0)) return;
    ev_run(loop);
}

void ev_break(EvLoop* loop)
{
    loop->loop_done = 1;
}

void ev_wakeup(EvLoop* loop)
{
    loop->reactor->wakeup(loop);
}

}

