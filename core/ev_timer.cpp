#include "ev_process_internal.h"

#include <stdlib.h>
#include <assert.h>

#include <new>

namespace evnet
{

namespace
{

inline void f(void* obj, int events)
{
    EvTimerCore* ev = (EvTimerCore*)obj;
    ev->cb(events, ev->data);
}

}

EvTimer* EvTimer::create()
{
    EvTimerCore* ev = new (std::nothrow) EvTimerCore();
    if (ev == 0)
        abort();

    ev->c.active     = 0;
    ev->c.pending    = 0;
    ev->c.priority   = EV_TIMEOUT_PRI;
    ev->c.f          = f;

    ev->t            = 0.;
    ev->repeat       = 0.;
    ev->cb           = 0;
    ev->data         = 0;
    ev->e._core = ev;

    return &ev->e;
}

void EvTimer::destroy(EvTimer* e)
{
    delete e->_core;
}

void EvTimer::set(ev_tstamp t, ev_tstamp repeat, EvFun cb, void* data)
{
    assert(t >= 0. && ("negative timer value"));
    assert(repeat >= 0. && ("negative ev_timer repeat value"));

    _core->t = t;
    _core->repeat = repeat;
    _core->cb = cb;
    _core->data = data;
}

void EvTimer::setRepeatTime(ev_tstamp t)
{
    assert(t >= 0. && ("negative ev_timer repeat value"));
    _core->repeat = t;
}

void EvTimer::setCb(EvFun cb)
{
    _core->cb = cb;
}

void EvTimer::setCbData(void* data)
{
    _core->data = data;
}

ev_tstamp EvTimer::time() const
{
    return _core->t;
}

ev_tstamp EvTimer::repeat() const
{
    return _core->repeat;
}

void* EvTimer::data() const
{
    return _core->data;
}

int EvTimer::isValid() const
{
    return (_core->c.active != 0);
}

void EvTimer::addTimer(EvLoop* loop)
{
    assert(_core != 0 && "EvTimer must be created by EvTimer::create");

    if (unlikely(_core->c.active != 0)) return;
    _loop = loop;

    assert(_loop != 0 && "EvLoop empty");

    ev_timer_add(_loop, _core);
}

void EvTimer::delTimer()
{
    assert(_core != 0 && "EvTimer must be created by EvTimer::create");
    if (unlikely(_core->c.active == 0)) return;
    ev_timer_del(_loop, _core);
    _core->c.active = 0;
}

}

