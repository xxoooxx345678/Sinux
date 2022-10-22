.section ".text.boot"

.global _start

_start:
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, master
    // cpu id > 0, stop
busyloop:  wfe
    b       busyloop
master:
    bl from_el2_to_el1

    adr x1, exception_vector_table
    msr vbar_el1, x1

    ldr     x1, =_start
    mov     sp, x1

    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
memloop:  
    cbz     w2, kernel
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, memloop

kernel:
    bl      main
    b       busyloop

from_el2_to_el1:
    ldr     x1, =0b11 << 20  // for printf
    msr     CPACR_EL1, x1
    mov x1, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x1
    mov x1, 0x3c5 // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x1
    msr elr_el2, lr
    eret // return to EL1

    
