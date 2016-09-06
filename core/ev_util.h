#ifndef __EV_UTIL_H__
#define __EV_UTIL_H__

#include <assert.h>

namespace evnet
{

#if defined(__GNUC__) && __GNUC__ >= 4
    #define likely(x)   (__builtin_expect((x), 1))
    #define unlikely(x) (__builtin_expect((x), 0))
#else
    #define likely(x)   (x)
    #define unlikely(x) (x)
#endif

#define array_needsize(type, base, cur, cnt, init) \
    if (unlikely((cnt) > (cur))) \
    { \
        int ocur_ = (cur); \
        (base) = (type *)array_realloc \
            (sizeof(type), (base), &(cur), (cnt)); \
        assert (base != 0 && "no memory"); \
        init ((base) + (ocur_), (cur) - ocur_); \
    }

void* array_realloc (int elem, void* base, int* cur, int cnt);
void* ev_realloc(void* ptr, long size);
int array_nextsize (int elem, int cur, int cnt);

#define ev_malloc(size) ev_realloc (0, (size))
#define ev_free(ptr)    ev_realloc ((ptr), 0)

}

#endif
