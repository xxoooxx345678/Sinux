#include <mm/mm.h>

#define ALIGN 4

extern char __heap_start[];

static char *heap_top = __heap_start;

void *smalloc(size_t size)
{
    size = (size & (ALIGN - 1) ? (size & ~(ALIGN - 1)) + ALIGN : size);
    heap_top += size;
    return (void *)(heap_top - size);
}