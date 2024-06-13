.section ".text"

.global _start

_start:
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, init

busyloop:
    wfe
    b       busyloop

init:
    // stack pointer
    mov     sp, 0x80000

kernel:
    bl      main