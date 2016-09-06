#ifndef __HTTP_RESPONSE_H__
#define __HTTP_RESPONSE_H__

#include "stream.h"
#include "paw_specification.h"

class HttpResponse
{
    public:
        explicit HttpResponse(PawOutput* output):
            _output(output)
        {
            _stream.init(output->buffer(), output->bufferLen());
        }

        ~HttpResponse()
        {
        }

        int parse(int fd)
        {
            int ret = _stream.read(fd);
            if (ret <= 0) return -1;
            return getResponse() == 1 ? 1 : 0;
        }

    private:
        int getResponse();

    private:
        PawOutput* _output;
        ReadStream _stream;
};

#endif

