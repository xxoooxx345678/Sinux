#include <mm/mmu.h>

.section ".text"

.global _start
.global dtb_base_address

_start:
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, init

busyloop:
    wfe
    b       busyloop
    
init:
    // save dtb_base_address
    mov     x29, x0

    // EL2 to EL1
    bl      from_el2_to_el1

    // stack pointer
    mov     sp, 0x3c000000

    b       mmu_init

boot_rest:
    // set exception vector table
    ldr     x0, =exception_vector_table
    msr     vbar_el1, x0

    // Stop trapping when accessing advanced SIMD and floating-point registers at EL0 & EL1
    mrs     x1, cpacr_el1
    orr     x1, x1, #(0b11 << 20)
    msr     cpacr_el1, x1

    // clear bss section
    bl      bssinit

    // pass dtb base address
    ldr     x0, =dtb_base_address
    str     x29, [x0]    

kernel:
    bl      main

from_el2_to_el1:
    mov     x0, (1 << 31)   // EL1 uses aarch64
    msr     hcr_el2, x0
    mov     x0, 0x3c5       // EL1h (SPSel = 1) with interrupt disabled
    msr     spsr_el2, x0
    msr     elr_el2, lr
    eret                    // return to EL1

mmu_init:
    ldr     x0, =TCR_CONFIG_DEFAULT
    msr     tcr_el1, x0

    ldr     x0, =((MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) | (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8)))
    msr     mair_el1, x0

    bl      setup_kernel_pagetables

    mrs     x0, sctlr_el1
    orr     x0 , x0, 1
    msr     sctlr_el1, x0

    movz    x0, 0x0000
    movk    x0, 0x3c00, lsl 16
    movk    x0, 0xffff, lsl 48
    mov     sp, x0

    ldr     x0, =boot_rest
    br      x0
