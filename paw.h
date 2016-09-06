#ifndef __PAW_H__
#define __PAW_H__

#include "ev.h"

#include <assert.h>

class PawSpecification;
class Paw
{
    public:
        Paw():
            _reactor(evnet::ev_init(evnet::EV_REACTOR_EPOLL))
        {
            assert(_reactor != 0);
        }

        virtual ~Paw()
        {
            ev_destroy(_reactor);
        }

        int execute(PawSpecification* spec);

    private:
        class Feeler;

    private:
        evnet::EvLoop* _reactor;
};

#endif

