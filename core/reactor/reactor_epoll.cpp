#include "reactor_epoll.h"
#include "ev_process_internal.h"

#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

namespace evnet
{

void ReactorEpoll::epoll_modify(EvLoop* loop, int fd, int old_flag, int new_flag)
{
    ReactorEpoll* reactor = (ReactorEpoll*)loop->reactor;
    if (new_flag == 0)
    {
        epoll_ctl(reactor->_epoll_fd, EPOLL_CTL_DEL, fd, 0);
        return;
    }

    struct epoll_event ev;
    ev.data.u64 = (uint64_t)(uint32_t)fd
                | ((uint64_t)(uint32_t)++loop->fds[fd].mgen << 32);
    ev.events = (new_flag & EV_IO_READ  ? EPOLLIN  : 0)
              | (new_flag & EV_IO_WRITE ? EPOLLOUT : 0);

    if (likely(!epoll_ctl(reactor->_epoll_fd,
                    old_flag && old_flag!=new_flag ? EPOLL_CTL_MOD : EPOLL_CTL_ADD,
                    fd,
                    &ev)))
        return;

    if (likely(errno == ENOENT))
    {
        if (!epoll_ctl(reactor->_epoll_fd, EPOLL_CTL_ADD, fd, &ev))
            return;
    }
    else if (likely(errno == EEXIST))
    {
        if (old_flag == new_flag)
            goto dec_egen;

        if (!epoll_ctl(reactor->_epoll_fd, EPOLL_CTL_MOD, fd, &ev))
            return;
    }

    fd_error(loop, fd);

dec_egen:
    /* we didn't successfully call epoll_ctl, so decrement the generation counter again */
    --loop->fds[fd].mgen;

    return;
}

void ReactorEpoll::epoll_poll(EvLoop* loop, int timeout)
{
    ReactorEpoll* reactor = (ReactorEpoll*)loop->reactor;
    int ev_cnt = epoll_wait(reactor->_epoll_fd,
            reactor->_events,
            reactor->_event_max,
            timeout);

    if (unlikely(ev_cnt < 0))
    {
        if (errno != EINTR)
            assert(0 && "(evnet) epoll_wait");
        return;
    }

    int i = 0;
    while (i < ev_cnt)
    {
        struct epoll_event* ev = reactor->_events + i;
        int fd = (uint32_t)ev->data.u64;
        if (fd == reactor->_pipe_write)
        {
            //if (likely((uint32_t)loop->wakeup.mgen == (uint32_t)(ev->data.u64 >> 32)))
                //loop->wakeup.flag = 1;
            ++i;
            continue;
        }
        int got = (ev->events & EPOLLOUT ? EV_IO_WRITE : 0)
                | (ev->events & EPOLLIN  ? EV_IO_READ  : 0)
                | (ev->events & (EPOLLERR | EPOLLHUP) ? EV_IO_RDWR | EV_IO_ERROR : 0);

        if (unlikely((uint32_t)loop->fds[fd].mgen != (uint32_t)(ev->data.u64 >> 32)))
        {
            ++i;
            continue;
        }

        fd_event(loop, fd, got);

        ++i;
    }

    if (unlikely(ev_cnt == reactor->_event_max))
    {
        ev_free(reactor->_events);
        reactor->_event_max = array_nextsize(sizeof(struct epoll_event),
                reactor->_event_max,
                reactor->_event_max+1);
        reactor->_events = (struct epoll_event*)ev_malloc(sizeof(struct epoll_event)*reactor->_event_max);
    }

    return;
}

void ReactorEpoll::epoll_wakeup(EvLoop* loop)
{
    ReactorEpoll* reactor = (ReactorEpoll*)loop->reactor;
    struct epoll_event ev;
    ev.data.u64 = (uint64_t)(uint32_t)reactor->_pipe_write;
                //| ((uint64_t)(uint32_t)++loop->wakeup.mgen << 32);
    ev.events = EPOLLOUT | EPOLLET;
    epoll_ctl(reactor->_epoll_fd, EPOLL_CTL_MOD, reactor->_pipe_write, &ev);
}

ReactorEpoll::ReactorEpoll():
    Reactor("epoll"),
    _epoll_fd(-1),
    _event_max(64),
    _events(0)
{
}

ReactorEpoll::~ReactorEpoll()
{
    close(_epoll_fd);
    close(_pipe_read);
    close(_pipe_write);

    if (_events)
        ev_free(_events);
}

int ReactorEpoll::init()
{
    int fd = epoll_create(256);
    if (fd < 0)
        return -1;

    int pfd[2];
    assert(pipe(pfd) == 0);

    fcntl(fd, F_SETFD, FD_CLOEXEC);
    fcntl(pfd[0], F_SETFD, FD_CLOEXEC);
    fcntl(pfd[1], F_SETFD, FD_CLOEXEC);

    _epoll_fd  = fd;
    _pipe_read = pfd[0];
    _pipe_write = pfd[1];
    _event_max = 64;
    _events = (struct epoll_event*)ev_malloc(sizeof(struct epoll_event) * _event_max);
    memset(_events, 0, sizeof(struct epoll_event) * _event_max);

    struct epoll_event ev;
    ev.data.u64 = (uint64_t)(uint32_t)_pipe_write;
    ev.events = EPOLLET;
    epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _pipe_write, &ev);

    _modify  = epoll_modify;
    _poll    = epoll_poll;
    _wakeup  = epoll_wakeup;

    return 0;
}

}

