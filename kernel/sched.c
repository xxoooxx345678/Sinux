#include <kernel/sched.h>
#include <kernel/timer.h>
#include <kernel/exception.h>
#include <drivers/uart.h>
#include <fs/cpio.h>
#include <mm/vm.h>
#include <mm/mmu.h>
#include <mm/mm.h>
#include <string.h>

extern signal_handler_t default_signal_handler_table[MAX_SIGNAL_HANDLER_CNT];

LIST_HEAD(run_queue);
LIST_HEAD(wait_queue);
thread_t threads[MAX_THREAD_COUNT];
thread_t *cur_thread = NULL;

void sched_init()
{
    for (int i = 0; i < MAX_THREAD_COUNT; ++i)
    {
        threads[i].pid = i;
        threads[i].status = OPEN;
    }

    asm volatile("msr   tpidr_el1, %0"::"r"(&(threads[0].context)));
    cur_thread = &threads[0];
    threads[0].status = RUNNING;
    list_add(&threads[0].listhead, &run_queue);
}

void sched_enable()
{
    CRITICAL_SECTION_START;
    timer_add(sched_enable, "", 1, (get_clock_freq() >> 5));
    CRITICAL_SECTION_END;
}

thread_context_t *get_current_thread_context_t()
{
    void *ret;
    
    asm volatile("mrs   %0, tpidr_el1" : "=r"(ret));

    return (thread_context_t *)((char *)ret);
}

static void kill_zombies()
{
    CRITICAL_SECTION_START;

    list_head_t *it = NULL;
    list_for_each(it, &run_queue)
    {
        thread_t *thread_ptr = (thread_t *)it;
        if (thread_ptr->status == DEAD)
        {
            list_del_entry(it);
            clear_mapping(thread_ptr);
            free(PHYS_TO_VIRT(thread_ptr->context.ttbr0_el1));
            free(thread_ptr->kernel_sp);
            thread_ptr->status = OPEN;
        }
    }

    CRITICAL_SECTION_END;
}

void schedule()
{
    if (list_size(&run_queue) > 1)
    {
        CRITICAL_SECTION_START;
        thread_t *next_thread = cur_thread;

        do
        {
            next_thread = (thread_t *)next_thread->listhead.next;
        } while (list_is_head(&next_thread->listhead, &run_queue) || next_thread->status == DEAD || next_thread->status == RUNNING);

        cur_thread->status = (cur_thread->status == DEAD) ? DEAD : READY;
        next_thread->status = RUNNING;

        cur_thread = next_thread;
        CRITICAL_SECTION_END;
        switch_to(get_current_thread_context_t(), &next_thread->context);
    }
}

void idle()
{
    while (1)
    {
        kill_zombies();
        schedule();
    }
}

thread_t *thread_create(program_t entry_point, const char *name)
{
    CRITICAL_SECTION_START;

    thread_t *thread_ptr;

    for (int i = 0; i < MAX_THREAD_COUNT; ++i)
    {
        if (threads[i].status == OPEN)
        {
            thread_ptr = &threads[i];
            break;
        }
    }

    strcpy(thread_ptr->name, name);
    thread_ptr->entry_point = entry_point;
    thread_ptr->status = READY;
    thread_ptr->kernel_sp = malloc(KSTACK_SIZE);
    thread_ptr->context.lr = entry_point;
    thread_ptr->context.sp = (uint64_t)thread_ptr->kernel_sp + KSTACK_SIZE;
    thread_ptr->context.fp = thread_ptr->context.sp;

    INIT_LIST_HEAD(&thread_ptr->vma);
    thread_ptr->context.ttbr0_el1 = VIRT_TO_PHYS((uint64_t)malloc(PAGE_FRAME_SIZE));

    memset(thread_ptr->registered_signal_handler, 0, sizeof(signal_handler_t) * MAX_SIGNAL_HANDLER_CNT);
    memset(thread_ptr->signal_pending_count, 0, sizeof(size_t) * MAX_SIGNAL_HANDLER_CNT);

    list_add(&thread_ptr->listhead, &run_queue);

    CRITICAL_SECTION_END;

    return thread_ptr;
}

int thread_exec(const char *name, char *const argv[])
{
    char *program_start = get_file_start(name);
    
    if (program_start == NULL)
        goto fail;
    CRITICAL_SECTION_START;

    cur_thread->kernel_sp = malloc(KSTACK_SIZE);
    cur_thread->context.sp = 0xfffffffff000;
    cur_thread->context.fp = 0xfffffffff000;

    /* Don't know why it is buggy, offset of some instructions is not correct! */
    // cur_thread->entry_point = (program_t)program_start;

    cur_thread->program_size = get_file_size(name);
    cur_thread->entry_point = malloc(cur_thread->program_size);
    memcpy(cur_thread->entry_point, program_start, cur_thread->program_size);

    clear_mapping(cur_thread);

    /* text section mapping */
    mappages(cur_thread, 0, cur_thread->program_size, (uint64_t)VIRT_TO_PHYS((char *)cur_thread->entry_point), PROT_READ | PROT_WRITE | PROT_EXEC);
    /* stack section mapping */
    mappages(cur_thread, 0xffffffffb000, 0x4000, 0, PROT_READ | PROT_WRITE | PROT_EXEC);
    /* mailbox mapping */
    mappages(cur_thread, 0x3c000000, 0x3000000, 0x3c000000, PROT_READ | PROT_WRITE);
    /* signal handler mapping */
    mappages(cur_thread, USER_SIG_WRAPPER_VIRT_ADDR_ALIGNED, PAGE_FRAME_SIZE * 2, (uint64_t)VIRT_TO_PHYS(registered_signal_handler_wrapper), PROT_READ | PROT_EXEC);
    /* signal user stack mapping */
    mappages(cur_thread, USER_SIG_STACK_VIRT_ADDR_ALIGNED, PAGE_FRAME_SIZE, 0, PROT_READ | PROT_WRITE | PROT_EXEC);

    CRITICAL_SECTION_END;
    run_program(0, 0xfffffffff000, cur_thread->kernel_sp + KSTACK_SIZE);
fail:
    return -1;
}

void thread_handle_signal()
{
    /* Nested signal handling is not allowed */
    if (cur_thread->handling_signal != NULL)
        return;

    for (int i = 0; i < MAX_SIGNAL_HANDLER_CNT; ++i)
    {
        store_context(&cur_thread->signal_context);
        if (cur_thread->signal_pending_count[i] > 0)
        {
            CRITICAL_SECTION_START;
            cur_thread->signal_pending_count[i]--;
            CRITICAL_SECTION_END;

            if (cur_thread->registered_signal_handler[i] != NULL)
            {
                cur_thread->handling_signal = cur_thread->registered_signal_handler[i];

                asm volatile("mov x16, %0"::"r"(cur_thread->handling_signal));
                run_program(USER_SIG_WRAPPER_VIRT_ADDR_ALIGNED + ((uint64_t)registered_signal_handler_wrapper % PAGE_FRAME_SIZE), USER_SIG_STACK_VIRT_ADDR_ALIGNED + PAGE_FRAME_SIZE, cur_thread->signal_context.sp);
            }
            else
            {
                cur_thread->handling_signal = default_signal_handler_table[i];
                cur_thread->handling_signal(0);
            }
        }
    }

    cur_thread->handling_signal = NULL;
}

void thread_exit()
{
    CRITICAL_SECTION_START;
    cur_thread->status = DEAD;
    CRITICAL_SECTION_END;
    while (1);
}