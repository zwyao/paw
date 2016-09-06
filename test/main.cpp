#include "paw.h"
#include "paw_specification.h"

#include <sys/time.h>

int main(int argc, char** argv)
{
    while (true)
    {
        PawSpecification spec;

        for (int i = 0; i < 8; ++i)
        {
            PawInput* input = spec.add_input();
            assert(input);

            //input->setHost("127.0.0.1", 8080);
            input->setHost(argv[1], atoi(argv[2]));
            input->timeout(atoi(argv[3]));

            input->add("GET / HTTP/1.1");
            input->add("Accept", "*/*");
            input->add("");

            //input->print();

            PawOutput* output = spec.add_output();
            assert(output);
        }

        Paw paw;
        //sleep(1);
        struct timeval t1, t2;
        gettimeofday(&t1, 0);
        paw.execute(&spec);
        gettimeofday(&t2, 0);

        fprintf(stderr, "%d ms\n", (t2.tv_sec-t1.tv_sec)*1000 + (t2.tv_usec-t1.tv_usec)/1000);

        /*
        for (int i = 0; i < 8; ++i)
        {
            fprintf(stderr, "ok: %d\n", spec.output(i)->dataSize());
        }
        */
    }
}

