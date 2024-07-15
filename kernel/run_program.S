.section ".text"

.global run_program

// argument
// x0: program start address
// x1: user stack start address
// x2: kernel stack start address
run_program:        
    msr     elr_el1, x0
    msr     spsr_el1, xzr
    msr     sp_el0, x1
    mov     sp, x2

    dsb ish // ensure write has completed
    tlbi vmalle1is // invalidate all TLB entries
    dsb ish // ensure completion of TLB invalidatation
    isb // clear pipeline
    
    eret