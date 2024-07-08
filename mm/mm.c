#include <mm/mm.h>

// #define DEMO

#define PAGE_FRAME_SIZE 0x1000
#define PAGE_FRAME_MAX_ORDER 8
#define CHUNK_SLOT_TYPE_COUNT 9
#define MEMORY_ALIGN 4

extern char __simple_allocator_start[];
extern char __simple_allocator_end[];
extern char __image_start[];
extern char __image_end[];
extern char* cpio_start;
extern char* cpio_end;
static char *simple_allocator_top = __simple_allocator_start;

static size_t pf_count;
static void *pf_start;

pf_entry_t *pf_entries;
list_head_t pf_freelists[PAGE_FRAME_MAX_ORDER];

const int chunk_slot_type[] = {
    16, 32, 64, 128, 256, 512, 1024, 2048, 4096
};
cs_entry_t *cs_entries;
list_head_t cs_freelists[CHUNK_SLOT_TYPE_COUNT];

void *smalloc(size_t size)
{
    size = (size & (MEMORY_ALIGN - 1) ? (size & ~(MEMORY_ALIGN - 1)) + MEMORY_ALIGN : size);
    simple_allocator_top += size;
    return (void *)(simple_allocator_top - size);
}

static void *pf_index_to_address(int index)
{
    return pf_start + index * PAGE_FRAME_SIZE;
}

static int pf_address_to_index(void *address)
{
    return (int)((address - pf_start) / PAGE_FRAME_SIZE);
}

void memory_reserve(void *start, void *end)
{
    start = (void *)((uint64_t)start / PAGE_FRAME_SIZE * PAGE_FRAME_SIZE);
    end = (void *)(((uint64_t) end + PAGE_FRAME_SIZE - 1) / PAGE_FRAME_SIZE * PAGE_FRAME_SIZE);

    for (void *tmp = start; tmp < end; tmp = (void *)((uint64_t)tmp + PAGE_FRAME_SIZE))
        pf_entries[pf_address_to_index(tmp)].status = ALLOCATED;
}

void pf_init()
{
    pf_entries = smalloc(sizeof(pf_entry_t) * pf_count);

    for (int i = 0; i < pf_count; ++i)
    {
        pf_entries[i] = (pf_entry_t){
            .index = i,
            .order = 0,
            .status = FREE
        };
    }

    // Reserve Pages
    memory_reserve(0x0000, 0x1000);
    memory_reserve(__image_start, __image_end);
    memory_reserve(cpio_start, cpio_end);
    memory_reserve(__simple_allocator_start, __simple_allocator_end);

    // Merge page frames
    for (int i = 0; i < PAGE_FRAME_MAX_ORDER - 1; ++i)
    {
        int pf1 = 0, pf2 = 0;
        while (1)
        {
            pf2 = pf1 ^ (1 << i);

            if (pf2 >= pf_count)
                break;

            if (pf_entries[pf1].status == FREE && pf_entries[pf2].status == FREE &&
                pf_entries[pf1].order == i && pf_entries[pf2].order == i)
                ++pf_entries[pf1].order;
            
            pf1 += (1 << (i + 1));

            if (pf1 >= pf_count)
                break;
        }
    }

    for (int i = 0; i < PAGE_FRAME_MAX_ORDER; ++i)
        INIT_LIST_HEAD(&pf_freelists[i]);

    for (int i = 0, order = 0; i < pf_count; i += (1 << order))
    {
        order = pf_entries[i].order;
        if (pf_entries[i].status == FREE)
        {
            list_add_tail((list_head_t *)pf_index_to_address(i), &pf_freelists[order]);

#ifdef DEMO
            uart_printf("%x added to %d'th order list\n", i, order);
#endif
        }
    }
}

void *pf_allocate(size_t n)
{
    if (n == 0)
        return NULL;
    
    int order = 0;

    while ((1ULL << order) < n)
        ++order;

    int actual_order = order;

    while (list_empty(&pf_freelists[actual_order]))
        ++actual_order;
    
    if (actual_order == PAGE_FRAME_MAX_ORDER)
    {
        uart_printf("No page available\n");
        return NULL;
    }

#ifdef DEMO
    uart_printf("ideal page order: %d\n", order);
    uart_printf("alloc page order: %d\n", actual_order);
#endif

    pf_entry_t *victim = &pf_entries[pf_address_to_index(pf_freelists[actual_order].next)];
    list_del_entry((list_head_t *)pf_freelists[actual_order].next);

#ifdef DEMO
    uart_printf("alloc page index: %d\n", victim->index);
#endif

    while (actual_order > order)
    {
        int buddy_index = victim->index ^ (1 << --actual_order);
        pf_entries[buddy_index].order = actual_order;
        pf_entries[buddy_index].status = FREE;

#ifdef DEMO
        uart_printf("released redundant page index: %d\n", buddy_index);
        uart_printf("released redundant page order: %d\n", actual_order);
#endif

        list_add_tail((list_head_t *)pf_index_to_address(buddy_index), &pf_freelists[actual_order]);
    }

    victim->order = order;
    victim->status = ALLOCATED;

#ifdef DEMO
    uart_printf("page is allocated at: %x\n", pf_index_to_address(victim->index));
    uart_printf("--------------------------------------------\n");
#endif
    return pf_index_to_address(victim->index);
}

void pf_free(void *address)
{
    int index = pf_address_to_index(address);
    int order = pf_entries[index].order;

    pf_entries[index].status = FREE;
    list_add_tail((list_head_t *)address, &pf_freelists[order]);

    int buddy_index = index ^ (1 << order);

    while (order < PAGE_FRAME_MAX_ORDER - 1 && pf_entries[buddy_index].status == FREE && pf_entries[buddy_index].order == order)
    {
        list_del_entry((list_head_t *)pf_index_to_address(index));
        list_del_entry((list_head_t *)pf_index_to_address(buddy_index));

        index &= buddy_index;
        pf_entries[index].order = ++order;
        list_add_tail((list_head_t *)pf_index_to_address(index), &pf_freelists[order]);
        buddy_index = index ^ (1 << order);
    }
}

void cs_init()
{
    cs_entries = smalloc(sizeof(cs_entry_t) * pf_count);

    for (int i = 0; i < pf_count; ++i)
        cs_entries[i].status = FREE;

    for (int i = 0; i < CHUNK_SLOT_TYPE_COUNT; ++i)
        INIT_LIST_HEAD(&cs_freelists[i]);
}

void *cs_allocate(size_t size)
{
    int type_index;

    for (type_index = 0; type_index < CHUNK_SLOT_TYPE_COUNT; ++type_index)
        if (chunk_slot_type[type_index] >= size)
            break;

#ifdef DEMO
        uart_printf("alloc chunk slot size: %d\n", chunk_slot_type[type_index]);
#endif
    
    if (list_empty(&cs_freelists[type_index]))
    {
        void *page = pf_allocate(1);
        int page_index = pf_address_to_index(page);

#ifdef DEMO
        uart_printf("Need page allocation !!! \n");
        uart_printf("Split page %d into chunks of size %d\n", page_index, chunk_slot_type[type_index]);
#endif

        cs_entries[page_index].type = type_index;
        cs_entries[page_index].status = ALLOCATED;

        for (int i = 0; i + chunk_slot_type[type_index] <= PAGE_FRAME_SIZE; i += chunk_slot_type[type_index])
        {
            list_head_t *cs_pointer = (list_head_t *)((char *)page + i);
            list_add_tail(cs_pointer, &cs_freelists[type_index]);
        }
    }

    void *ret = (void *)cs_freelists[type_index].next;

#ifdef DEMO
    uart_printf("chunk is allocated at: %x\n", cs_freelists[type_index].next);
    uart_printf("--------------------------------------------\n");
#endif
    list_del_entry(cs_freelists[type_index].next);

    return ret;
}

void cs_free(void *address)
{
    int index = pf_address_to_index(address);

    list_add_tail((list_head_t *)address, &cs_freelists[cs_entries[index].type]);
}

void *malloc(size_t size)
{
    CRITICAL_SECTION_START;
    void *ret;
    if (size < PAGE_FRAME_SIZE)
    {
#ifdef DEMO
        uart_printf("--------------------------------------------\n");
        uart_printf("size: %d\n", size);
#endif
        ret = cs_allocate(size);
    }
    else
    {
#ifdef DEMO
        uart_printf("--------------------------------------------\n");
        uart_printf("size: %d, page num: %d\n", size, (size + PAGE_FRAME_SIZE - 1) / PAGE_FRAME_SIZE);
#endif
        ret = pf_allocate((size + PAGE_FRAME_SIZE - 1) / PAGE_FRAME_SIZE);
    }
    CRITICAL_SECTION_END;
    return ret;
}

void free(void *address)
{
    CRITICAL_SECTION_START;
    int index = pf_address_to_index(address);

    if (pf_entries[index].status != FREE)
    {
        if (cs_entries[index].status == FREE)
            pf_free(address);
        else
            cs_free(address);
    }
    CRITICAL_SECTION_END;
}

void memory_init(void *start, void *end)
{
    pf_start = start;
    pf_count = (end - start) / PAGE_FRAME_SIZE;

    cs_init();
    pf_init();
}

void page_frame_allocator_test()
{
    char *pages[10];

    pages[0] = malloc(1*PAGE_FRAME_SIZE + 123);
    pages[1] = malloc(1*PAGE_FRAME_SIZE);
    pages[2] = malloc(3*PAGE_FRAME_SIZE + 321);
    pages[3] = malloc(3*PAGE_FRAME_SIZE + 31);
    pages[4] = malloc(3*PAGE_FRAME_SIZE + 21);
    pages[5] = malloc(3*PAGE_FRAME_SIZE);

    free(pages[2]);
    pages[6] = malloc(1*PAGE_FRAME_SIZE);
    pages[7] = malloc(1*PAGE_FRAME_SIZE + 333);
    pages[8] = malloc(1*PAGE_FRAME_SIZE);

    // Merge blocks
    free(pages[6]);
    free(pages[7]);
    free(pages[8]);

    // free all
    free(pages[0]);
    free(pages[1]);
    free(pages[3]);
    free(pages[4]);
    free(pages[5]);
}

void chunk_slot_allocator_test()
{
    char *chunk[100];
    
    int tmp = PAGE_FRAME_SIZE / 512;

    for (int i = 0; i <= tmp; ++i)
        chunk[i] = malloc(0x101);
    
    chunk[tmp + 1] = malloc(0x11);
    chunk[tmp + 2] = malloc(0x15);

    chunk[tmp + 3] = malloc(0x25);
    chunk[tmp + 4] = malloc(0x35);

    for (int i = 0; i <= tmp; ++i)
        cs_free(chunk[i]);

    free(chunk[tmp + 1]);
    free(chunk[tmp + 2]);
    free(chunk[tmp + 3]);
    free(chunk[tmp + 4]);
}