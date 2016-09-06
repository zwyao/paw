#include "paw.h"
#include "paw_specification.h"
#include "socket.h"
#include "stream.h"
#include "http_response.h"
#include "ev_io.h"
#include "ev_timer.h"
#include "ev_util.h"

#include <sys/time.h>

class Paw::Feeler
{
    public:
        enum ConnectionStatus
        {
            CONNECTING,
            WRITE,
            READ,
            ERROR,
            MAX,
        };

        static const char* status[MAX+1];

    public:
        Feeler(const PawInput* input,
                PawOutput* output,
                evnet::EvLoop* reactor):
            _input(input),
            _output(output),
            _reactor(reactor),
            _io(evnet::EvIo::create()),
            _timer(evnet::EvTimer::create()),
            _response(_output),
            _done(0)
        {
        }

        ~Feeler()
        {
            evnet::EvIo::destroy(_io);
            evnet::EvTimer::destroy(_timer);
            _sock.close();
        }

        int init()
        {
            int ret = _sock.open(_input->host().c_str(), _input->port());
            if (ret != 0) return -1;

            _status = CONNECTING;

            _io->setEvent(_sock.fd(), evnet::EV_IO_WRITE, processor, this);
            _io->addEvent(_reactor);

            evnet::ev_tstamp t = _input->timeout()*0.001;
            _timer->set(t, 0, processor, this);
            _timer->addTimer(_reactor);

            return 0;
        }

        int done() const { return _done; }

    private:
        const PawInput* _input;
        PawOutput* _output;

        evnet::EvLoop* _reactor;
        evnet::EvIo* _io;
        evnet::EvTimer* _timer;

        WriteStream _write_stream;
        HttpResponse _response;

        Socket _sock;
        int _done;
        int _status;

    private:
        inline void process_timeout();
        inline void process_io();
        inline int process_connect();
        int process_write();
        int process_read();

    private:
        static void processor(int event, void* data);
};

const char* Paw::Feeler::status[MAX+1] = {
    "connecting",
    "write",
    "read",
    "error",
    0
};

void Paw::Feeler::processor(int event, void* data)
{
    Paw::Feeler* feeler = (Paw::Feeler*)data;
    if (event & evnet::EV_TIMER)
    {
        feeler->process_timeout();
        return;
    }

    feeler->process_io();
}

void Paw::Feeler::process_timeout()
{
    _io->delEvent();
    _timer->delTimer();
    _output->payload(0, 0);

    fprintf(stderr, "%s:%d timeout [%s]\n", _input->host().c_str(),
            _input->port(),
            Paw::Feeler::status[_status]);
}

void Paw::Feeler::process_io()
{
    int ret = -1;
    switch(_status)
    {
        case CONNECTING:
            ret = process_connect();
            break;
        case WRITE:
            ret = process_write();
            break;
        case READ:
            ret = process_read();
            break;
        default:
            assert(0);
    }

    if (ret == -1)
    {
        _io->delEvent();
        _timer->delTimer();
        _output->payload(0, 0);

        fprintf(stderr, "%s:%d error [%s]\n", _input->host().c_str(),
                _input->port(),
                Paw::Feeler::status[_status]);
    }
}

int Paw::Feeler::process_connect()
{
    int val = -1;
    socklen_t lon;
    lon = sizeof(int);
    getsockopt(_sock.fd(), SOL_SOCKET, SO_ERROR, &val, &lon);
    if (val == 0)
    {
        _write_stream.init(_input->data(), _input->dataSize());
        _status = WRITE;
        return 0;
    }

    return -1;
}

int Paw::Feeler::process_write()
{
    int ret = _write_stream.write(_sock.fd());
    if (ret < 0) return -1;

    if (_write_stream.isWriteFinished())
    {
        _status = READ;
        _io->modEvFlag(EV_IO_READ);
    }

    return 0;
}

int Paw::Feeler::process_read()
{
    int ret = _response.parse(_sock.fd());
    if (ret == 1)
    {
        _io->delEvent();
        _timer->delTimer();
        _done = 1;
    }
    else if (ret < 0)
    {
        return -1;
    }

    return 0;
}

int Paw::execute(PawSpecification* spec)
{
    int input_num = spec->inputNum();
    int output_num = spec->outputNum();

    assert(input_num == output_num);

    Paw::Feeler* request[input_num];
    int i = 0;
    int active = 0;
    while (i < input_num)
    {
        Paw::Feeler* t = new (std::nothrow) Paw::Feeler(spec->input(i),
                spec->output(i),
                _reactor);

        if (t && t->init() == 0)
        {
            request[i] = t;
            ++active;
        }
        else
        {
            request[i] = 0;
            delete t;
        }

        ++i;
    }

    if (active)
    {
        ev_loop(_reactor);

        i = 0;
        while (i < input_num)
        {
            if (request[i]) delete request[i];
            ++i;
        }
    }

    return active;;
}

