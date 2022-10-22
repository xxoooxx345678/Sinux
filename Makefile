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

A_SRCS = $(wildcard kernel/lib/*.S)
C_SRCS = $(wildcard kernel/lib/*.c)
OBJS = $(A_SRCS:.S=.o) $(C_SRCS:.c=.o)
INCLUDE = kernel/include
CFLAGS = -ggdb -Wno-implicit -ffreestanding -nostdlib -nostartfiles -I$(INCLUDE) -DDEMO
CPIO_SRC = $(wildcard rootfs/*)
CPIO_FILE = initramfs.cpio
USR_SRC = $(wildcard programs/*.S)
USR_FILE = rootfs/$(notdir $(basename $(USR_SRC)))
USR_OBJS = $(USR_SRC:.S=.o)
DTB_FILE = bcm2710-rpi-3-b-plus.dtb

IMAGE = kernel8

.PHONY: clean

all: $(IMAGE).img $(USR_FILE) $(CPIO_FILE) $(DTB_FILE)

$(IMAGE).img: kernel8.elf
	$(OBJCPY) -O binary $(IMAGE).elf kernel8.img

$(IMAGE).elf: $(OBJS)
	$(LK) $(OBJS) -T kernel/linker.ld -o $(IMAGE).elf

$(USR_FILE): $(USR_OBJS)
	$(LK) $(USR_OBJS) -T programs/linker.ld -o programs/$(notdir $(USR_FILE)).elf
	$(OBJCPY) -O binary programs/$(notdir $(USR_FILE)).elf $(USR_FILE)

$(CPIO_FILE): $(CPIO_FILE) $(USR_FILE)
	cd rootfs && (find . | cpio -o -H newc > ../initramfs.cpio)

kernel/%.o: kernel/lib/%.S
	$(CC) -c $< $(CFLAGS) -o $@

kernel/%.o: kernel/lib/%.c
	$(CC) -c $< $(CFLAGS) -o $@

programs/%.o: programs/%.S
	$(CC) -c $< $(CFLAGS) -o $@

programs/%.o: programs/%.c
	$(CC) -c $< $(CFLAGS) -o $@

run: $(IMAGE).img $(CPIO_FILE)
	$(QEMU) -machine $(MACHINE) -kernel $(IMAGE).img -initrd $(CPIO_FILE) -dtb $(DTB_FILE) -display none -serial null -serial stdio

dbg: $(IMAGE).img $(CPIO_FILE)
	$(QEMU) -machine $(MACHINE) -kernel $(IMAGE).img -initrd $(CPIO_FILE) -dtb $(DTB_FILE) -display none -serial null -serial stdio -s -S

clean:
	rm -rf $(OBJS) *.img *.elf $(CPIO_FILE) $(USR_OBJS) $(USR_FILE) */*.elf 


