OS := $(shell uname)

ifeq ($(OS), Linux)
	ARMGNU = aarch64-linux-gnu
	MACHINE = raspi3
endif

ifeq ($(OS), Darwin)
	ARMGNU = aarch64-unknown-linux-gnu
	MACHINE = raspi3b
endif

CC = $(ARMGNU)-gcc
LK = $(ARMGNU)-ld
OBJCPY = $(ARMGNU)-objcopy
QEMU = qemu-system-aarch64

A_SRCS = $(wildcard lib/*.S)
C_SRCS = $(wildcard lib/*.c)
OBJS = $(A_SRCS:.S=.o) $(C_SRCS:.c=.o)
INCLUDE = include
CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles -I$(INCLUDE)
CPIO_FILE = ../initramfs.cpio
DTB_FILE = ../bcm2710-rpi-3-b-plus.dtb
IMAGE = bootloader

.PHONY: clean

all: $(IMAGE).img

$(IMAGE).img: $(IMAGE).elf
	$(OBJCPY) -O binary $(IMAGE).elf $(IMAGE).img

$(IMAGE).elf: $(OBJS)
	$(LK) $(OBJS) -T linker.ld -o $(IMAGE).elf

%.o: lib/%.S
	$(CC) -c $< $(CFLAGS) -o $@

%.o: lib/%.c
	$(CC) -c $< $(CFLAGS) -o $@

tty: $(IMAGE).img
	$(QEMU) -machine $(MACHINE) -kernel $(IMAGE).img -initrd $(CPIO_FILE) -dtb $(DTB_FILE) -display none -serial null -serial pty

clean:
	rm -rf $(OBJS) *.img *.elf


