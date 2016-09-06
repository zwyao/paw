#ifndef __EV_LOOP_H__
#define __EV_LOOP_H__

namespace evnet
{

typedef void (*EvFun) (int event, void* data);
typedef void (*WakeupFun) (void* data);

//all flags
enum
{
    EV_IO_NONE        = 0x00000000U,
    EV_IO_READ        = 0x00000001U,
    EV_IO_WRITE       = 0x00000002U,
    EV_IO_RDWR        = 0x00000003U,
    EV_IO_ERROR       = 0x00000004U,
    EV_IO_FDSET       = 0x00000080U,
    EV_IO_MASK        = 0x000000FFU,
    EV_TIMER          = 0x00000100U,
    EV_REACTOR_SELECT = 0x00010000U,
    EV_REACTOR_POLL   = 0x00020000U,
    EV_REACTOR_EPOLL  = 0x00030000U,
    EV_REACTOR_MASK   = 0x00030000U,
    EV_ERROR          = 0x40000000U,
};

#define REACTOR_FLAG(flag)          ((flag) & EV_REACTOR_MASK)
#define IS_REACTOR_SELECT(flag)     REACTOR_FLAG(flag) == EV_REACTOR_SELECT 
#define IS_REACTOR_POOL(flag)       REACTOR_FLAG(flag) == EV_REACTOR_POLL
#define IS_REACTOR_EPOLL(flag)      REACTOR_FLAG(flag) == EV_REACTOR_EPOLL

#define EV_MAXPRI      6
#define EV_ERROR_PRI   5
#define EV_TIMEOUT_PRI 4
#define EV_HIGH_PRI    3
#define EV_NORMAL_PRI  2
#define EV_MINPRI      1
#define NUMPRI (EV_MAXPRI-EV_MINPRI+1)

typedef double ev_tstamp;

struct EvWakeup
{
    unsigned char mgen;
    unsigned int flag;
    WakeupFun f;
    void* data;
};

struct EvFd;
struct EvPending;
struct EvTimerHeapItem;
struct EvCore;

class Reactor;
struct EvLoop
{
    int loop_done;

    int pending_pri;
    int pending_max[NUMPRI];
    int pending_cnt[NUMPRI];
    EvPending* pendings[NUMPRI];

    int fd_changes_max;
    int fd_changes_cnt;
    int* fd_changes;

    int fd_error_max;
    int fd_error_cnt;
    int* fd_error;

    int max_fds;
    EvFd* fds;

    int active_cnt;

    int timer_cnt;
    int timer_max;
    EvTimerHeapItem* timers;
    ev_tstamp now;

    EvCore* ev_dummy;
    Reactor* reactor;
    EvWakeup wakeup;
};

EvLoop* ev_init(unsigned int flags);

inline void ev_set_wakeup(EvLoop* loop, WakeupFun f, void* data)
{
    loop->wakeup.f = f;
    loop->wakeup.data = data;
}

void ev_destroy(EvLoop* loop);
void ev_loop(EvLoop* loop);
void ev_break(EvLoop* loop);
void ev_wakeup(EvLoop* loop);

}

#endif
