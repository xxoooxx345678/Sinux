#include <mm/vm.h>
#include <mm/mm.h>
#include <mm/mmu.h>
#include <drivers/uart.h>
#include <kernel/sched.h>
#include <kernel/exception.h>
#include <string.h>

extern thread_t *cur_thread;

void setup_kernel_pagetables()
{
    uint64_t *PGD = (uint64_t *)0x1000;
    uint64_t *PUD = (uint64_t *)0x2000;
    uint64_t *PMD = (uint64_t *)0x3000;
    uint64_t *PMD2 = (uint64_t *)0x4000;

    *PGD = (uint64_t)PUD | BOOT_PGD_ATTR;
    *PUD = (uint64_t)PMD | BOOT_PUD_ATTR;
    *(PUD + 1) = (uint64_t)PMD2 | BOOT_PUD_ATTR;

    for (int i = 0; i < 512; ++i)
    {
        if (0x200000UL * i >= 0x3f000000L)
            PMD[i] = (0x200000UL * i) | BOOT_PMD_DEVICE_ATTR;
        else
            PMD[i] = (0x200000UL * i) | BOOT_PMD_MEMORY_ATTR;
    }

    for (int i = 0; i < 512; ++i)
        PMD2[i] = (0x40000000UL + (0x200000UL * i)) | BOOT_PMD_DEVICE_ATTR;

    asm volatile("msr   ttbr0_el1, %0"::"r"(PGD));
    asm volatile("msr   ttbr1_el1, %0"::"r"(PGD));
}

void *walk(void *pgd, const uint64_t va, int alloc)
{
    uint64_t pt_indices[4] = {
        PGD_INDEX(va),
        PUD_INDEX(va),
        PMD_INDEX(va),
        PTE_INDEX(va),
    };

    uint64_t *pt = (uint64_t *)pgd;
    uint64_t pt_index;

    for (int i = 0; i < 3; ++i)
    {
        pt_index = pt_indices[i];

        if (pt[pt_index] == 0)
        {
            if (alloc)
            {
                uint64_t *new_pt = malloc(PAGE_FRAME_SIZE);
                memset(new_pt, 0, PAGE_FRAME_SIZE);
                pt[pt_index] = VIRT_TO_PHYS((uint64_t)new_pt);
                pt[pt_index] |= USER_TABLE;
            }
            else
                return NULL;
        }

        pt = (uint64_t *)PHYS_TO_VIRT((uint64_t)(pt[pt_index] & PD_ADDR_MASK));
    }

    pt_index = pt_indices[3];
    return &pt[pt_index];
}

static void mappage(void *pgd, uint64_t va, uint64_t pa, int prot)
{
    /* Align to page size */
    va &= ~(PAGE_FRAME_SIZE - 1);
    pa &= ~(PAGE_FRAME_SIZE - 1);

    uint64_t *pte = (uint64_t *)walk(pgd, va, 1);

    uint64_t perm = 0;

    if (!(prot & PROT_EXEC))
        perm |= PD_UXN;
    if (!(prot & PROT_WRITE))
        perm |= PD_RDONLY;
    if (prot & PROT_READ)
        perm |= PD_UK_ACCESS;

    *pte =  pa | perm | USER_PAGE;

    inc_ref_count((void *)(PHYS_TO_VIRT(pa)));
}

void *mappages(thread_t *thread, uint64_t va, size_t size, uint64_t pa, int prot)
{
    CRITICAL_SECTION_START;
    /* Calculate address of start & end pages */
    uint64_t va_start = va & ~(PAGE_FRAME_SIZE - 1);
    uint64_t va_end = (va + size + PAGE_FRAME_SIZE - 1) & ~(PAGE_FRAME_SIZE - 1);

    /* add vma */
    int overlapped = 0;
    list_head_t *it;
    do {
        list_for_each(it, &thread->vma)
        {
            vm_area_t *vma_ptr = (vm_area_t *)it;
            if (!((vma_ptr->end <= va_start) || (va_end <= vma_ptr->start)))
            {
                overlapped = 1;

                uint64_t offset = va_end - va_start;
                va_start = vma_ptr->end >= 0xfffffffff000 ? 0 : vma_ptr->end;
                va_end = va_start + offset;

                break;
            }
        }
        overlapped = 0;
    } while (overlapped);

    vm_area_t *vma = malloc(sizeof(vm_area_t));    
    INIT_LIST_HEAD(&vma->listhead);
    vma->start = va_start;
    vma->end = va_end;
    vma->prot = prot;
    vma->freeable = 1;

    list_add(&vma->listhead, &thread->vma);
    CRITICAL_SECTION_END;

    /* wait for page fault to allocate page frame */
    if (pa == 0)
        return (void *)va_start;

    CRITICAL_SECTION_START;
    vma->freeable = 0;

    uint64_t start_pa = pa & ~(PAGE_FRAME_SIZE - 1);
    uint64_t end_pa = (pa + size + PAGE_FRAME_SIZE - 1) & ~(PAGE_FRAME_SIZE - 1);

    for (uint64_t _va = va_start, _pa = start_pa; _va < va_end; _va += PAGE_FRAME_SIZE, _pa += PAGE_FRAME_SIZE)
        mappage(PHYS_TO_VIRT(thread->context.ttbr0_el1), _va, _pa, prot);

    CRITICAL_SECTION_END;
    return (void *)va_start;
}

static void free_pagetables(void *pt, int level)
{
    uint64_t *_pt = (uint64_t *)pt;

    for (int i = 0; i < 512; ++i)
    {
        if ((_pt[i] & 1) == 0)
            continue;

        uint64_t _next_pt = PHYS_TO_VIRT(_pt[i] & PD_ADDR_MASK);
        void *next_pt = (void *)_next_pt;
        if (level == 3)
        {
            dec_ref_count(next_pt);
            if (get_ref_count(next_pt) == 0)
                free(next_pt);
        }
        else
        {
            free_pagetables(next_pt, level + 1);
            _pt[i] = 0L;
            free(next_pt);
        }
    }
}

void clear_mapping(thread_t *thread)
{
    list_head_t *it, *its;
    CRITICAL_SECTION_START;

    list_for_each_safe(it, its, &thread->vma)
    {
        list_del_entry(it);
        free(it);
    } 

    asm volatile("dsb ish");
    free_pagetables(PHYS_TO_VIRT(thread->context.ttbr0_el1), 0);
    memset(PHYS_TO_VIRT(thread->context.ttbr0_el1), 0, 0x1000);
    asm volatile("tlbi vmalle1is");
    asm volatile("dsb ish");
    asm volatile("isb");
    CRITICAL_SECTION_END;
}

static void copy_pagetables(void *dst, void *src, int level)
{
    uint64_t *_src = (uint64_t *)src;
    uint64_t *_dst = (uint64_t *)dst;

    for (int i = 0; i < 512; ++i)
    {
        if ((_src[i] & 1) == 0)
            continue;

        uint64_t _src_next_pt = PHYS_TO_VIRT(_src[i] & PD_ADDR_MASK);
        void *src_next_pt = (void *)_src_next_pt;

        if (level == 3)
        {
            if (copyable_page_frame(src_next_pt))
            {
                void *new_pt = malloc(PAGE_FRAME_SIZE);
                memcpy(new_pt, src_next_pt, PAGE_FRAME_SIZE);
                _dst[i] = VIRT_TO_PHYS((uint64_t)new_pt);
                _dst[i] |= (_src[i] & ~PD_ADDR_MASK);
            }
            else
                _dst[i] = _src[i];
            inc_ref_count((void *)(PHYS_TO_VIRT(_dst[i] & PD_ADDR_MASK)));
        }
        else
        {
            void *new_pt = malloc(PAGE_FRAME_SIZE);
            memset(new_pt, 0, PAGE_FRAME_SIZE);

            _dst[i] = VIRT_TO_PHYS((uint64_t)new_pt) | USER_TABLE;
            copy_pagetables(new_pt, src_next_pt, level + 1);
        }
    }
}

void copy_mapping(thread_t *dst, thread_t *src)
{
    list_head_t *it;
    
    list_for_each(it, &src->vma)
    {
        vm_area_t *old_vma = (vm_area_t *)it;
        vm_area_t *new_vma = malloc(sizeof(vm_area_t));
        new_vma->start = old_vma->start; 
        new_vma->end = old_vma->end; 
        new_vma->prot = old_vma->prot; 
        new_vma->freeable = old_vma->freeable; 

        list_add(&new_vma->listhead, &dst->vma);
    }

    copy_pagetables(PHYS_TO_VIRT(dst->context.ttbr0_el1), PHYS_TO_VIRT(src->context.ttbr0_el1), 0);
}

vm_area_t *find_vma(uint64_t addr)
{
    vm_area_t *target_vma = NULL;
    list_head_t *it;    
    list_for_each(it, &cur_thread->vma)
    {
        vm_area_t *vma_ptr = (vm_area_t *)it;
        if (addr >= vma_ptr->start && addr <= vma_ptr->end)
        {
            target_vma = vma_ptr;
            break;
        }
    } 

    return target_vma;
}

void page_fault_handler(exception_t *ex)
{
    uint64_t page_fault_address;
    asm volatile("mrs %0, far_el1":"=r"(page_fault_address));

    vm_area_t *target_vma = find_vma(page_fault_address);

    if (target_vma == NULL)
        goto seg_fault;
    
    if ((ex->iss & 0x3f) == TRANS_FAULT_LV0 || (ex->iss & 0x3f) == TRANS_FAULT_LV1 || (ex->iss & 0x3f) == TRANS_FAULT_LV2 || (ex->iss & 0x3f) == TRANS_FAULT_LV3)
    {
        uart_printf("[Translation fault]: 0x%x\n", page_fault_address);
        mappage(PHYS_TO_VIRT(cur_thread->context.ttbr0_el1), page_fault_address & ~(PAGE_FRAME_SIZE - 1), (uint64_t)VIRT_TO_PHYS((char *)malloc(PAGE_FRAME_SIZE)), target_vma->prot);
        return;
    }

seg_fault:
    uart_printf("[Segmentation fault]: 0x%x Kill process\n", page_fault_address);
    thread_exit();
}