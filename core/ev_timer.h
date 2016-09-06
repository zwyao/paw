#ifndef __EV_TIMER_H__
#define __EV_TIMER_H__

namespace evnet
{

struct EvTimerCore;
struct EvLoop;
class EvTimer
{
    public:
        EvTimer():
            _core(0),
            _loop(0)
        {}

        ~EvTimer() {}

    public:
        /*
         * 通过create来构造EvTimer
         */
        static EvTimer* create();
        static void destroy(EvTimer* e);

    public:
        /*
         * @t: 第一次超时时间
         * @repeat: 超时时间间隔,如果0，第一次超时之后，卸载这个timer;如果>0,用repeat重新设置超时
         */
        void set(ev_tstamp t, ev_tstamp repeat, EvFun cb, void* data);

        /*
         * 设置间隔时间
         * 下次超时后生效
         */
        void setRepeatTime(ev_tstamp t);

        /*
         * 设置回调函数
         * 实时生效
         */
        void setCb(EvFun cb);

        /*
         * 设置回调函数可能用到的数据
         * 实时生效
         */
        void setCbData(void* data);

        ev_tstamp time() const;
        ev_tstamp repeat() const;
        void* data() const;

        /*
         * 是否添加到驱动中
         */
        int isValid() const;

        void addTimer(EvLoop* loop);
        /*
         * 退出事件驱动,驱动中不会保留它的任何痕迹
         * 但是自身的数据未改动
         */
        void delTimer();

    private:
        EvTimerCore* _core;
        EvLoop* _loop;
};

}

#endif
