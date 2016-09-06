#include "ev_process_internal.h"
#include "reactor_epoll.h"

#include <assert.h>
#include <stdio.h>
#include <sys/time.h>

namespace evnet
{

namespace
{

#define EV_ANFD_REIFY 1
#define ABSPRI(e) ((e)->priority - EV_MINPRI)
#define MALLOC_ROUND 4096 /* prefer to allocate in chunks of this size, must be 2**n and >> 4 longs */

inline void up_heap(EvTimerHeapItem* heap, int k)
{
    EvTimerHeapItem that = heap[k];
    int p = k >> 1;
    while (p > 0)
    {
        if (heap[p].t <= that.t)
            break;
        heap[k] = heap[p];
        heap[k].e->c.active = k;

        k = p;
        p = k >> 1;
    }

    that.e->c.active = k;
    heap[k] = that;
}

inline void down_heap(EvTimerHeapItem* heap, int n, int k)
{
    EvTimerHeapItem that = heap[k];
    int j = k << 1;
    while (j <= n)
    {
        if (j < n && heap[j].t > heap[j+1].t)
            ++j;

        if (that.t <= heap[j].t)
            break;

        heap[k] = heap[j];
        heap[k].e->c.active = k;
        k = j;
        j = k << 1;
    }

    that.e->c.active = k;
    heap[k] = that;
}

inline void adjust_heap(EvTimerHeapItem* heap, int n, int k)
{
    int p = k >> 1;
    if (p > 0 && heap[p].t > heap[k].t)
        up_heap(heap, k);
    else
        down_heap(heap, n, k);
}

inline void invoke_pending(EvLoop* loop)
{
    int pending_pri = NUMPRI;
    while (pending_pri)
    {
        --pending_pri;
        EvPending* ep = loop->pendings[pending_pri];

        while (loop->pending_cnt[pending_pri])
        {
            EvPending* p = ep + --loop->pending_cnt[pending_pri];
            p->e->pending = 0;
            p->e->f(p->e, p->events);
        }
    }
}

inline void clear_pending(EvLoop* loop, EvCore* e)
{
    if (e->pending)
    {
        int pri = ABSPRI(e);
        loop->pendings[pri][e->pending-1].e = loop->ev_dummy;
        loop->pendings[pri][e->pending-1].events = 0;
        e->pending = 0;
    }
}

inline void ev_feed_event(EvLoop* loop, EvCore* e, int revents)
{
    int pri = ABSPRI(e);

    if (unlikely(e->pending))
        loop->pendings[pri][e->pending - 1].events |= revents;
    else
    {
        e->pending = ++loop->pending_cnt[pri];
        array_needsize(EvPending,
                loop->pendings[pri],
                loop->pending_max[pri],
                e->pending,
                EMPTY2);

        loop->pendings[pri][e->pending - 1].e      = e;
        loop->pendings[pri][e->pending - 1].events = revents;
    }
}

inline void fd_kill(EvLoop* loop)
{
    EvIoCore* e;
    int i = 0;
    while (i < loop->fd_error_cnt)
    {
        int fd = loop->fd_error[i];
        while ((e = loop->fds[fd].head))
        {
            e->e.delEvent();
            e->c.priority = EV_ERROR_PRI;
            ev_feed_event(loop, &(e->c), EV_ERROR|EV_IO_RDWR);
        }

        ++i;
    }

    loop->fd_error_cnt = 0;
}

/**
 * 重新设置fd的监听事件
 */
inline void fd_reify(EvLoop* loop)
{
    int i = 0;
    while (i < loop->fd_changes_cnt)
    {
        int fd = loop->fd_changes[i];
        EvFd* anfd = loop->fds + fd;

        unsigned char o_events = anfd->events;
        unsigned char o_reify  = anfd->reify;

        anfd->reify = 0;
        anfd->events = 0;

        EvIoCore* e = anfd->head;
        while (e)
        {
            anfd->events |= (unsigned char)e->events;
            e = e->next;
        }

        if (o_events != anfd->events)
            o_reify = EV_IO_FDSET; /* actually |= */

        if (o_reify & EV_IO_FDSET)
            loop->reactor->modity(loop, fd, o_events, anfd->events);

        ++i;
    }

    loop->fd_changes_cnt = 0;

    if (unlikely(loop->fd_error_cnt > 0))
        fd_kill(loop);
}

inline void timer_reify(EvLoop* loop)
{
    while (loop->timer_cnt && loop->timers[1].t < loop->now)
    {
        EvTimerCore* e = loop->timers[1].e;
        if (e->repeat)
        {
            loop->timers[1].t += e->repeat;
            if (loop->timers[1].t < loop->now)
                loop->timers[1].t = loop->now;

            down_heap(loop->timers, loop->timer_cnt, 1);
        }
        else
        {
            e->e.delTimer();
        }

        ev_feed_event(loop, &(e->c), EV_TIMER);
    }
}

inline void fd_change(EvLoop* loop, int fd, int flags)
{
    unsigned char reify = loop->fds[fd].reify;
    loop->fds[fd].reify |= flags;

    if (likely(!reify))
    {
        array_needsize(int,
                loop->fd_changes,
                loop->fd_changes_max,
                loop->fd_changes_cnt+1,
                EMPTY2);
        loop->fd_changes[loop->fd_changes_cnt] = fd;
        ++loop->fd_changes_cnt;
    }
}

}

ev_tstamp ev_time()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

inline void time_update(EvLoop* loop)
{
    ev_tstamp prev_now = loop->now;
    ev_tstamp rt_now = ev_time();
    if (unlikely(rt_now < prev_now))
    {
        int i = 1;
        int adjust = rt_now - prev_now;
        while (i <= loop->timer_cnt)
        {
            loop->timers[i].t += adjust;
            ++i;
        }
    }
    loop->now = rt_now;
    /*
    //epoll 时间精度毫秒
    if (loop->timer_cnt &&
            loop->timers[1].t > loop->now &&
            loop->timers[1].t - loop->now < 0.001)
        loop->now = loop->timers[1].t;
    */
}

inline void adjust_timer_when_run(EvLoop* loop)
{
    ev_tstamp prev_now = loop->now;
    ev_tstamp rt_now = ev_time();

    int i = 1;
    while (i <= loop->timer_cnt)
    {
        //fprintf(stderr, "%f -> %f\n", loop->timers[i].t, loop->timers[i].t - prev_now + rt_now);
        loop->timers[i].t = loop->timers[i].t - prev_now + rt_now;
        ++i;
    }

    loop->now = rt_now;
}

ev_tstamp get_timeout(EvLoop* loop)
{
    time_update(loop);

    ev_tstamp waittime = 59.73;
    if (loop->timer_cnt)
    {
        ev_tstamp to = loop->timers[1].t - loop->now;
        if (waittime > to) waittime = to;
    }

    //fprintf(stderr, "%f\n", waittime);
    //epoll 时间精度毫秒,最少1毫秒
    if (waittime < 0.001) waittime = 0.001;

    if (unlikely(loop->pending_cnt[EV_ERROR_PRI-1] && waittime > 0.1))
        waittime = 0.1;

    return waittime;
}

int array_nextsize (int elem, int cur, int cnt)
{
    int ncur = cur + 1;

    do
        ncur <<= 1;
    while (cnt > ncur);

    /* if size is large, round to MALLOC_ROUND - 4 * longs to accommodate malloc overhead */
    if (elem * ncur > MALLOC_ROUND - (int)sizeof(void *) * 4)
    {
        ncur *= elem;
        ncur = (ncur + elem + (MALLOC_ROUND - 1) + sizeof (void *) * 4) & ~(MALLOC_ROUND - 1);
        ncur = ncur - sizeof (void *) * 4;
        ncur /= elem;
    }

    return ncur;
}

void* ev_realloc(void* ptr, long size)
{
    if (size) 
        ptr = realloc(ptr, size);
    else
    {
        free(ptr);
        ptr = 0;
    }

    if (!ptr && size)
        abort ();

    return ptr;
}

void* array_realloc(int elem, void* base, int* cur, int cnt)
{
    *cur = array_nextsize(elem, *cur, cnt);
    return ev_realloc(base, elem * *cur);
}

void ev_run(EvLoop* loop)
{
    adjust_timer_when_run(loop);

    while (likely(loop->active_cnt && !loop->loop_done))
    {
        // 上半部分
        // 添加IO事件
        // change event read/write
        fd_reify(loop);
        ev_tstamp timeout = get_timeout(loop);
        loop->reactor->poll(loop, timeout*1e3);
        // 下半部分
        // 添加超时事件
        time_update(loop);
        timer_reify(loop);

        invoke_pending(loop);

        /*
        if (loop->wakeup.flag)
        {
            if (loop->wakeup.f) loop->wakeup.f(loop->wakeup.data);
            loop->wakeup.flag = 0;
        }
        */
    }// while (likely(loop->active_cnt && !loop->loop_done));
}

void fd_error(EvLoop* loop, int fd)
{
    array_needsize(int,
            loop->fd_error,
            loop->fd_error_max,
            loop->fd_error_cnt+1,
            EMPTY2);
    loop->fd_error[loop->fd_error_cnt] = fd;
    ++loop->fd_error_cnt;
}

void fd_event(EvLoop* loop, int fd, int event)
{
    if (likely(loop->fds[fd].reify == 0))
    {
        EvIoCore* e = loop->fds[fd].head;
        while (e)
        {
            if (e->events & event)
                ev_feed_event(loop, &(e->c), event);

            e = e->next;
        }
    }
}

void ev_io_add(EvLoop* loop, EvIoCore* e)
{
    int fd = e->fd;
    array_needsize(EvFd,
            loop->fds,
            loop->max_fds,
            fd+1,
            array_init_zero);
    e->next = loop->fds[fd].head;
    loop->fds[fd].head = e;
    ++loop->active_cnt;
    fd_change(loop, fd, (e->events & EV_IO_FDSET) | EV_ANFD_REIFY);
    e->events &= ~EV_IO_FDSET;
}

void ev_io_del(EvLoop* loop, EvIoCore* e)
{
    clear_pending(loop, &(e->c));

    int fd = e->fd;

    assert(fd >= 0 && fd < loop->max_fds && "invalid fd");

    EvIoCore** p = &loop->fds[fd].head;
    while (*p)
    {
        if (likely(*p == e))
        {
            *p = e->next;
            break;
        }

        p = &(*p)->next;
    }

    --loop->active_cnt;
    fd_change(loop, fd, EV_ANFD_REIFY);
}

void ev_io_mod(EvLoop* loop, EvIoCore* e)
{
    fd_change(loop, e->fd, EV_ANFD_REIFY);
}

void ev_timer_add(EvLoop* loop, EvTimerCore* e)
{
    ++loop->timer_cnt;
    array_needsize(EvTimerHeapItem,
            loop->timers,
            loop->timer_max,
            loop->timer_cnt+1,
            EMPTY2);
    e->c.active = loop->timer_cnt;
    loop->timers[e->c.active].t = e->t + loop->now;
    loop->timers[e->c.active].e = e;
    ++loop->active_cnt;
    up_heap(loop->timers, e->c.active);
}

void ev_timer_del(EvLoop* loop, EvTimerCore* e)
{
    clear_pending(loop, &(e->c));

    int k = e->c.active;

    assert(k <= loop->timer_cnt);
    assert(e == loop->timers[k].e);

    --loop->timer_cnt;
    --loop->active_cnt;

    if (likely(k <= loop->timer_cnt))
    {
        loop->timers[k] = loop->timers[loop->timer_cnt+1];
        loop->timers[k].e->c.active = k;
        adjust_heap(loop->timers, loop->timer_cnt, k);
    }
}

}

