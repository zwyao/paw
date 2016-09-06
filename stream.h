#ifndef __STREAM_H__
#define __STREAM_H__

#include <unistd.h>

class WriteStream
{
    public:
        WriteStream():
            _buffer(0),
            _len(0),
            _pos(0)
        {
        }

        ~WriteStream() {}

        void init(const char* buffer, int len)
        {
            _buffer = buffer;
            _len = len;
            _pos = 0;
        }

        int write(int fd)
        {
            int ret = ::write(fd, _buffer+_pos, _len-_pos);
            if (ret >= 0)
            {
                _pos += ret;
                return 0;
            }
            else
            {
                return -1;
            }
        }

        int isWriteFinished() const
        {
            return _pos == _len ? 1 : 0;
        }

    private:
        const char* _buffer;
        int _len;
        int _pos;
};

class ReadStream
{
    public:
        ReadStream():
            _buffer(0),
            _len(0),
            _pos(0)
        {
        }

        ~ReadStream()
        {
        }

        void init(char* buffer, int len)
        {
            _buffer = buffer;
            _len = len;
        }

        int read(int fd)
        {
            int ret = ::read(fd, _buffer+_pos, _len-_pos);
            switch (ret)
            {
                case 0:
                    break;
                case -1:
                    break;
                default:
                    _pos += ret;
                    break;
            }

            return ret;
        }

        int byteSize() const { return _pos; }

    private:
        char* _buffer;
        int _len;
        int _pos;
};

#endif

