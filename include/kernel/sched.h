#ifndef SCHED_H
#define SCHED_H

#include <kernel/signal.h>
#include <kernel/list.h>
#include <kernel/trapframe.h>
#include <stddef.h>

#define MAX_THREAD_COUNT        256
#define MAX_THREAD_NAME_LEN     32
#define USTACK_SIZE             0x1000
#define KSTACK_SIZE             0x1000

// typedef void (*program_t)();
typedef char * program_t;

typedef enum thread_status {
    OPEN,
    READY,
    RUNNING,
    WAITING,
    DEAD,
} thread_status;

typedef struct thread_context_t {
    uint64_t x19;
    uint64_t x20;
    uint64_t x21;
    uint64_t x22;
    uint64_t x23;
    uint64_t x24;
    uint64_t x25;
    uint64_t x26;
    uint64_t x27;
    uint64_t x28;
    uint64_t fp;
    uint64_t lr;
    uint64_t sp;
    void *ttbr0_el1;
} thread_context_t;

typedef struct thread {
    /* Run queue, wait queue*/
    list_head_t listhead;

    /* Context*/
    thread_context_t context;

    /* Name */
    char name[MAX_THREAD_NAME_LEN];

    /* Program info */
    program_t entry_point;
    size_t program_size;

    /* Thread status */
    int pid;
    thread_status status;

    /* Stack pointer */
    void *kernel_sp;

    /* Trapframe */
    trapframe_t *trapframe;

    /* Memory management */
    list_head_t vma;

    /* Signal */
    signal_handler_t registered_signal_handler[MAX_SIGNAL_HANDLER_CNT];
    size_t signal_pending_count[MAX_SIGNAL_HANDLER_CNT];
    signal_handler_t handling_signal;
    thread_context_t signal_context;
} thread_t;

void sched_init();
void sched_enable();
void schedule();
void idle();

thread_t *thread_create(program_t entry_point, const char *name);
int thread_exec(const char *name, char *const argv[]);
void thread_handle_signal();
void thread_exit();


#endif