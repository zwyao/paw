/**
 * 线程不安全
 *
 * 应该在回调函数中调用
 *
 */

#ifndef __EV_IO_H__
#define __EV_IO_H__

namespace evnet
{

struct EvIoCore;
struct EvLoop;
class EvIo
{
    public:
        EvIo():
            _core(0),
            _loop(0)
        {}

        ~EvIo() {}

    public:
        /*
         * 通过create来构造EvIo
         */
        static EvIo* create();
        static void destroy(EvIo* e);

    public:
        void setEvent(int fd, int events, EvFun cb, void* data);

        /*
         * 设置IO事件，可以是
         * EV_IO_READ,EV_IO_WRITE,EV_IO_RDWR
         */
        void setEvFlag(int events);

        /*
         * 设置IO事件，可以是
         * EV_IO_READ,EV_IO_WRITE,EV_IO_RDWR
         *
         * 实时生效
         *
         * 线程不安全
         */
        void modEvFlag(int events);

        /*
         * 设置回调函数
         *
         * 实时生效
         */
        void setCb(EvFun cb);

        /*
         * 设置回调函数可能用到的数据
         *
         * 实时生效
         */
        void setCbData(void* data);

        /*
         * 设置优先级
         *
         * 实时生效
         */
        void setPriority(int pri);

        int fd() const;
        int events() const;
        void* data() const;

        /*
         * 是否添加到驱动中
         */
        int isValid() const;

        /*
         * 添加到驱动中
         * 线程不安全
         */
        void addEvent(int priority = 1);

        /*
         * 添加到驱动中
         * 线程不安全
         */
        void addEvent(EvLoop* loop, int priority = 1);

        /*
         * 退出事件驱动,驱动中不会保留它的任何痕迹
         * 但是自身的数据未改动
         *
         * 线程不安全
         */
        void delEvent();

    private:
        EvIoCore* _core;
        EvLoop* _loop;
        unsigned int _egen;
};

}

#endif
