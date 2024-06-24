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

    // set exception vector table
    ldr     x0, =exception_vector_table
    msr     vbar_el1, x0

    // stack pointer
    mov     sp, 0x3c000000

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
