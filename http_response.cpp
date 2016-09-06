#include "http_response.h"

int HttpResponse::getResponse()
{
    int bound = _stream.byteSize();
    char* head_bound = strstr(_output->buffer(), "\r\n\r\n");
    if (head_bound)
    {
        char* content_len = strcasestr(_output->buffer(), "Content-Length");
        assert(content_len);
        content_len += strlen("Content-Length");
        content_len = strchr(content_len, ':') + 1;
        int payload_len = atoi(content_len);
        head_bound += 4;
        int head_len = head_bound - _output->buffer();

        _output->payload(head_len, payload_len);

        int read_payload_len = bound - head_len;
        if (read_payload_len == payload_len)
            return 1;

        int pack_len = head_len + payload_len;
        if (pack_len > _output->bufferLen())
        {
            _output->expand();
            _stream.init(_output->buffer(), _output->bufferLen());
        }
    }
    else
    {
        if (bound == _output->bufferLen())
        {
            _output->expand();
            _stream.init(_output->buffer(), _output->bufferLen());
        }
    }

    return 0;
}

