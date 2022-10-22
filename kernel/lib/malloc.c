#include "malloc.h"

extern char _startup_allocator_start;
extern char _startup_allocator_end;
static char* top = &_startup_allocator_start;

void* smalloc(unsigned int size) {
    char *r = top;
    size = (size + 0x10 - 1) / 0x10 * 0x10;
    if ((unsigned long long int)top + size > (unsigned long long int)&_startup_allocator_end)
        return 0;
    top += size;
    return r;
}