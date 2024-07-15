#ifndef MM_H
#define MM_H

#include <stddef.h>

#define PAGE_FRAME_SIZE 0x1000

typedef enum {
    FREE,
    ALLOCATED,
    RESERVED
} mm_status;

typedef struct pf_entry {
    int index;
    int order;
    size_t ref_count;
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
void inc_ref_count(void *addr);
void dec_ref_count(void *addr);
size_t get_ref_count(void *addr);
int copyable_page_frame(void *addr);
void page_frame_allocator_test();
void chunk_slot_allocator_test();

#endif