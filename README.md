# Sinux

A simple kernel for bare-metal Raspberry Pi 3b+ (AArch64)

## Features

- Bootloader
- Memory management
    - Buddy system & chunk slots
- Interrupt handling
    - Timer interrupt
    - UART interrupt
    - Software interrupt
    - etc.
- Preemptive multi-threading 
- System call
    - exec
    - fork
    - mmap
    - signal
    - etc.
- Virtual Memory
- Virtual Filesystem

## TODO

- [ ] Provide support for user-space program
    - shell
    - ls
    - cat
    - User library
    
- [ ] Fix cpio bug (program has to be copy first otherwise it is buggy)

## Build & Run

### Requirements

- Cross compiler
    - ubuntu: **aarch64-linux-gnu-\***
    - MacOS: **aarch64-unknown-linux-gnu-\***

Noted that there might be a suffix for **aarch64-linux-gnu-\***, e.g. **aarch64-linux-gnu-gcc-12**, please check and modify the makefiles if there is conflicts.

- GNU Make
- qemu-system-aarch64

### Build

```
/Sinux # make
```

### Run

```
/Sinux # make run

or

# For fork & framebuffer test
/Sinux # make run_display
```

### Clean 

```
/Sinux # make clean
```

### Debug

```
# In Terminal 1

/Sinux # make dbg
```

```
# In Terminal 2

/Sinux # make gdb
```

## References
- [NYCU OSC2023](https://oscapstone.github.io/)  
- [xv6](https://github.com/mit-pdos/xv6-riscv)
