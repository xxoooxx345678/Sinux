.section ".text"

.global run_program

// argument
// x0: program start address
// x1: stack start address
run_program:        
    msr     elr_el1, x0
    msr     spsr_el1, xzr
    msr     sp_el0, x1
    eret