#ifndef __EV_PROCESS_INTERNAL_H__
#define __EV_PROCESS_INTERNAL_H__

#include "ev.h"
#include "ev_io.h"
#include "ev_timer.h"
#include "ev_util.h"

namespace evnet
{

#define array_init_zero(base, count) \
      memset ((void *)(base), 0, sizeof(*(base)) * (count))

#define EMPTY2(a, b)

#define get_priority(pri) \
    ((pri) <= EV_MAXPRI ? ((pri) >= EV_MINPRI ? (pri) : EV_MINPRI) : EV_MAXPRI)

void fd_error(EvLoop* loop, int fd);
void fd_event(EvLoop* loop, int fd, int event);
void ev_io_del(EvLoop* loop, EvIoCore* e);
void ev_io_add(EvLoop* loop, EvIoCore* e);
void ev_io_mod(EvLoop* loop, EvIoCore* e);
void ev_timer_add(EvLoop* loop, EvTimerCore* e);
void ev_timer_del(EvLoop* loop, EvTimerCore* e);
void ev_run(EvLoop* loop);
ev_tstamp ev_time();

typedef void (*FunWrapper) (void* obj, int events);

struct EvFd
{
    EvIoCore* head;
    unsigned char events;
    unsigned char reify;
    unsigned char unused;
    unsigned char mgen;
};

struct EvCore
{
    int active;
    int pending;
    int priority;
    FunWrapper f;
};

struct EvIoCore
{
    EvCore c;
    EvIoCore* next;
    int fd;
    int events;
    EvFun cb;
    void* data;
    EvIo e;
};

struct EvTimerCore
{
    EvCore c;
    ev_tstamp t;
    ev_tstamp repeat;
    EvFun cb;
    void* data;
    EvTimer e;
};

struct EvTimerHeapItem
{
    ev_tstamp t;
    EvTimerCore* e;
};

struct EvPending
{
    EvCore* e;
    int events;
};

}

#endif
