#ifndef __REACTOR_H__
#define __REACTOR_H__

#include <string.h>
#include <stdlib.h>

namespace evnet
{

class EvLoop;
typedef void (*reactor_modify) (EvLoop* loop, int fd, int old_flag, int new_flag);
typedef void (*reactor_poll)   (EvLoop* loop, int timeout);
typedef void (*reactor_wakeup) (EvLoop* loop);

class Reactor
{
    public:
        Reactor():
            _modify(0),
            _poll(0)
        {
            _name[0] = '\0';
        }

        explicit Reactor(const char* name):
            _modify(0),
            _poll(0)
        {
            strncpy(_name, name, 15);
            _name[15] = '\0';
        }

        virtual ~Reactor() {}

        const char* name() const { return _name; }

        inline void modity(EvLoop* loop, int fd, int old_flag, int new_flag)
        {
            return _modify(loop, fd, old_flag, new_flag);
        }

        inline void poll(EvLoop* loop, int timeout)
        {
            return _poll(loop, timeout);
        }

        inline void wakeup(EvLoop* loop)
        {
            return _wakeup(loop);
        }

        void validCheck()
        {
            if (_modify == 0 ||
                    _poll == 0)
                abort();
        }

    protected:
        char _name[16];

        reactor_modify  _modify;
        reactor_poll    _poll;
        reactor_wakeup  _wakeup;
};

}

#endif
