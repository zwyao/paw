#include "ev_process_internal.h"

#include <stdlib.h>
#include <assert.h>

#include <new>

#define EV_ACTIVE_MAGIC 0x4F495645 //"EVIO"

namespace evnet
{

namespace
{

inline void f(void* obj, int events)
{
    EvIoCore* ev = (EvIoCore*)obj;
    ev->cb(events, ev->data);
}

}

EvIo* EvIo::create()
{
    EvIoCore* ev = new (std::nothrow) EvIoCore();
    if (ev == 0)
        abort();

    ev->c.active     = 0;
    ev->c.pending    = 0;
    ev->c.priority   = EV_MINPRI;
    ev->c.f          = f;

    ev->next       = 0;
    ev->fd         = 0;
    ev->events     = 0;
    ev->cb         = 0;
    ev->data       = 0;
    ev->e._core = ev;

    return &ev->e;
}

void EvIo::destroy(EvIo* e)
{
    delete e->_core;
}

void EvIo::setEvent(int fd, int events, EvFun cb, void* data)
{
    _core->fd = fd;
    _core->events = events;
    _core->cb = cb;
    _core->data = data;
}

void EvIo::setEvFlag(int events)
{
    _core->events = events;
}

void EvIo::modEvFlag(int events)
{
    assert(_core != 0 && "EvIo must be created by EvIo::create");

    if (unlikely(_core->c.active != EV_ACTIVE_MAGIC))
        return;

    _core->events = events;
    ev_io_mod(_loop, _core);
}

void EvIo::setCb(EvFun cb)
{
    _core->cb = cb;
}

void EvIo::setCbData(void* data)
{
    _core->data = data;
}

void EvIo::setPriority(int pri)
{
    _core->c.priority = pri;
}

int EvIo::fd() const
{
    return _core->fd;
}

int EvIo::events() const
{
    return _core->events;
}

void* EvIo::data() const
{
    return _core->data;
}

int EvIo::isValid() const
{
    return (_core->c.active == EV_ACTIVE_MAGIC);
}

void EvIo::addEvent(int priority)
{
    assert(_core != 0 && "EvIo must be created by EvIo::create");

    if (unlikely(_core->c.active == EV_ACTIVE_MAGIC)) return;

    _core->c.active = EV_ACTIVE_MAGIC;
    _core->c.priority = get_priority(priority);
    _core->events |= EV_IO_FDSET;

    assert(_core->fd >= 0 && "negative fd");
    assert(!(_core->events & ~(EV_IO_FDSET|EV_IO_RDWR)) && "invalid event mask");
    assert(_loop != 0 && "EvLoop empty");

    ev_io_add(_loop, _core);
}

void EvIo::addEvent(EvLoop* loop, int priority)
{
    assert(_core != 0 && "EvIo must be created by EvIo::create");

    if (unlikely(_core->c.active == EV_ACTIVE_MAGIC)) return;

    _core->c.active = EV_ACTIVE_MAGIC;
    _core->c.priority = get_priority(priority);
    _core->events |= EV_IO_FDSET;
    _loop = loop;

    assert(_core->fd >= 0 && "negative fd");
    assert(!(_core->events & ~(EV_IO_FDSET|EV_IO_RDWR)) && "invalid event mask");
    assert(_loop != 0 && "EvLoop empty");

    ev_io_add(_loop, _core);
}

void EvIo::delEvent()
{
    assert(_core != 0 && "EvIo must be created by EvIo::create");
    if (unlikely(_core->c.active != EV_ACTIVE_MAGIC)) return;
    ev_io_del(_loop, _core);
    _core->c.active = 0;
}

}

