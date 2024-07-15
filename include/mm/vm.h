#ifndef VM_H
#define VM_H

#define USER_SIG_WRAPPER_VIRT_ADDR_ALIGNED 0xffffffff9000L
#define USER_SIG_STACK_VIRT_ADDR_ALIGNED 0xffffffff8000L
#define MAILBOX_VIRT_ADDR_ALIGNED 0x3c000000L

#include <kernel/list.h>
#include <kernel/sched.h>
#include <kernel/exception.h>
#include <stddef.h>

typedef struct vm_area {
    list_head_t listhead;
    uint64_t start;
    uint64_t end;
    int prot;
    int freeable;
} vm_area_t;

void *walk(void *pgd, const uint64_t va, int alloc);
/* if pa == 0, it does not map, wait for page fault to allocate a page frame for it */
void *mappages(thread_t *thread, uint64_t va, size_t size, uint64_t pa, int prot);
void clear_mapping(thread_t *thread);
void copy_mapping(thread_t *dst, thread_t *src);
vm_area_t *find_vma(uint64_t addr);
void page_fault_handler(exception_t *ex);

#endif