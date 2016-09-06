#ifndef __PAW_SPECIFICATION_H__
#define __PAW_SPECIFICATION_H__

#include "ev_util.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <new>
#include <string>

#define EMPTY2(a, b)

using namespace evnet;

class PawInput
{
    public:
        PawInput():
            _buffer(0),
            _buffer_len(0),
            _buffer_index(0)
        {
        }

        ~PawInput()
        {
            ev_free(_buffer);
        }

        PawInput* add(const char* key, const char* value)
        {
            int key_len = strlen(key);
            int value_len = strlen(value);
            // key:value\r\n
            int size = _buffer_index+key_len+value_len+5;
            array_needsize(char,
                    _buffer,
                    _buffer_len,
                    size,
                    EMPTY2);

            memcpy(_buffer+_buffer_index, key, key_len);
            _buffer_index += key_len;
            _buffer[_buffer_index++] = ':';
            _buffer[_buffer_index++] = ' ';

            memcpy(_buffer+_buffer_index, value, value_len);
            _buffer_index += value_len;
            _buffer[_buffer_index++] = '\r';
            _buffer[_buffer_index++] = '\n';
            _buffer[_buffer_index] = '\0';

            return this;
        }

        PawInput* add(const char* key)
        {
            int key_len = strlen(key);
            int size = _buffer_index+key_len+3;
            array_needsize(char,
                    _buffer,
                    _buffer_len,
                    size,
                    EMPTY2);

            memcpy(_buffer+_buffer_index, key, key_len);
            _buffer_index += key_len;
            _buffer[_buffer_index++] = '\r';
            _buffer[_buffer_index++] = '\n';
            _buffer[_buffer_index] = '\0';

            return this;
        }

        PawInput* setHost(const char* host, int port = 80)
        {
            _host.assign(host);
            _port = port;

            return this;
        }

        const std::string& host() const { return _host; }
        int port() const { return _port; }

        const char* data() const { return _buffer; }
        int dataSize() const { return _buffer_index; }

        void timeout(int ms) { _timeout = ms; }
        int timeout() const { return _timeout; }

        void print()
        {
            fprintf(stderr, "size:%d, index:%d\n", _buffer_len, _buffer_index);
            fprintf(stderr, "%s", _buffer);
        }

    private:
        char* _buffer;
        int _buffer_len;
        int _buffer_index;

        std::string _host;
        int _port;
        int _timeout;
};

class PawOutput
{
    private:
        static const int DEFAULT_LEN = 512;

    public:
        PawOutput():
            _buffer_len(DEFAULT_LEN)
        {
            _buffer = (char*)::malloc(_buffer_len);
            assert(_buffer);
        }

        ~PawOutput()
        {
            release();
        }

        void release()
        {
            ::free(_buffer);
        }

        void expand()
        {
            array_needsize(char,
                    _buffer,
                    _buffer_len,
                    2*_buffer_len,
                    EMPTY2);
        }

        void payload(int pos, int size)
        {
            _data_pos = pos;
            _data_size = size;
        }

        char* buffer() const { return _buffer; }
        int bufferLen() const { return _buffer_len; }

        /**
         * 与dataSize()配合使用
         * @return 返回数据的起始地址
         */
        const char* data() const { return _buffer+_data_pos; }

        /**
         * @return >0 输出的数据长度
         *         =0 未获取到数据
         */
        int dataSize() const { return _data_size; }

    private:
        char* _buffer;
        int _buffer_len;

        int _data_pos;
        int _data_size;
};

class PawSpecification
{
    private:
        static const int DEFAULT_INPUT_COUNT = 8;

    public:
        PawSpecification():
            _default_input_index(0),
            //_additional_input(0),
            //_additional_input_count(0),
            //_additional_input_index(0),
            _default_output_index(0)
            //_additional_output(0),
            //_additional_output_count(0),
            //_additional_output_index(0)
        {
        }

        ~PawSpecification()
        {
            /*
            if (_additional_input)
                delete [] _additional_input;

            if (_additional_output)
                delete [] _additional_output;
            */
        }

        PawInput* add_input()
        {
            if (likely(_default_input_index < DEFAULT_INPUT_COUNT))
                return &_default_input[_default_input_index++];

            return 0;

            /*
            if (unlikely(_additional_input == 0))
            {
                _additional_input_count = 2*DEFAULT_INPUT_COUNT;
                _additional_input = new (std::nothrow) PawInput [_additional_input_count];
            }

            if (unlikely(_additional_input == 0 ||
                        _additional_input_index >= _additional_input_count))
                return 0;

            return &_additional_input[_additional_input_index++];
            */
        }

        PawOutput* add_output()
        {
            if (likely(_default_output_index < DEFAULT_INPUT_COUNT))
                return &_default_output[_default_output_index++];

            return 0;

            /*
            if (unlikely(_additional_output == 0))
            {
                _additional_output_count = 2*DEFAULT_INPUT_COUNT;
                _additional_output = new (std::nothrow) PawOutput [_additional_output_count];
            }

            if (unlikely(_additional_output == 0 ||
                        _additional_output_index >= _additional_output_count))
                return 0;

            return &_additional_output[_additional_output_index++];
            */
        }

        int inputNum() const { return _default_input_index; }
        const PawInput* input(int index)
        {
            assert(index < _default_input_index);
            return &_default_input[index];
        }

        int outputNum() const { return _default_output_index; }
        PawOutput* output(int index)
        {
            assert(index < _default_output_index);
            return &_default_output[index];
        }

    private:
        PawInput _default_input[DEFAULT_INPUT_COUNT];
        int _default_input_index;

        /*
        PawInput* _additional_input;
        int _additional_input_count;
        int _additional_input_index;
        */

        PawOutput _default_output[DEFAULT_INPUT_COUNT];
        int _default_output_index;

        /*
        PawOutput* _additional_output;
        int _additional_output_count;
        int _additional_output_index;
        */
};

#endif

