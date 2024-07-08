#ifndef MM_H
#define MM_H

#include <kernel/list.h>
#include <kernel/exception.h>
#include <mm/mm.h>
#include <stddef.h>

typedef enum {
    FREE,
    ALLOCATED
} mm_status;

typedef struct pf_entry {
    int index;
    int order;
    mm_status status;
} pf_entry_t;

typedef struct cs_entry {
    int index;
    int type;
    mm_status status;
} cs_entry_t;

void *smalloc(size_t size);

void *malloc(size_t size);
void free(void *address);
void memory_init(void *start, void *end);

void page_frame_allocator_test();
void chunk_slot_allocator_test();

#endif