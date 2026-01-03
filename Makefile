CC ?= gcc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
LDFLAGS ?=
LDLIBS ?= -lm

SRC_DIR := src
TEST_DIR := tests
BUILD_DIR := build

# Path to a Linux kernel image to embed into the bootable ISO.
# Override when needed, e.g.: make iso KERNEL_IMAGE=/path/to/bzImage
KERNEL_IMAGE ?= /boot/vmlinuz-$(shell uname -r)
GRUB_MKRESCUE ?= grub-mkrescue

# Raspberry Pi boot packaging (expects Raspberry Pi firmware + kernel image)
RPI_FIRMWARE_DIR ?= /usr/lib/raspi-firmware
RPI_KERNEL_IMAGE ?= $(RPI_FIRMWARE_DIR)/kernel8.img
# Default to Pi 4B device tree; override for your model.
RPI_DTB ?= bcm2711-rpi-4-b.dtb
RPI_BOOT_DIR := $(BUILD_DIR)/rpi_boot

APP_SRCS := \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/kernel/kernel.c \
	$(SRC_DIR)/drivers/console_display.c \
	$(SRC_DIR)/drivers/console_keypad.c \
	$(SRC_DIR)/apps/calc_app.c \
	$(SRC_DIR)/calc/lexer.c \
	$(SRC_DIR)/calc/parser.c \
	$(SRC_DIR)/calc/eval.c \
	$(SRC_DIR)/calc/format.c \
	$(SRC_DIR)/platform/linux_poweroff.c \
	$(SRC_DIR)/util/strutil.c \
	$(SRC_DIR)/util/status.c

TEST_SRCS := \
	$(TEST_DIR)/test_main.c \
	$(SRC_DIR)/calc/lexer.c \
	$(SRC_DIR)/calc/parser.c \
	$(SRC_DIR)/calc/eval.c \
	$(SRC_DIR)/calc/format.c \
	$(SRC_DIR)/util/strutil.c \
	$(SRC_DIR)/util/status.c

APP_OBJS := $(patsubst %,$(BUILD_DIR)/%,$(APP_SRCS:.c=.o))
TEST_OBJS := $(patsubst %,$(BUILD_DIR)/%,$(TEST_SRCS:.c=.o))

INITRAMFS_INIT_SRC := $(SRC_DIR)/platform/initramfs_init.c
INITRAMFS_INIT_OBJ := $(patsubst %,$(BUILD_DIR)/%,$(INITRAMFS_INIT_SRC:.c=.o))

.PHONY: all clean run test qemu-initramfs qemu-run iso iso-run rpi-boot rpi-boot-tar

all: $(BUILD_DIR)/calc_os

$(BUILD_DIR)/calc_os: $(APP_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/test_runner: $(TEST_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c -o $@ $<

run: $(BUILD_DIR)/calc_os
	$(BUILD_DIR)/calc_os

$(BUILD_DIR)/calc_os_static: $(APP_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -static $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/initramfs_init_static: $(INITRAMFS_INIT_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -static $(LDFLAGS) -o $@ $^


$(BUILD_DIR)/initramfs.cpio.gz: $(BUILD_DIR)/calc_os_static $(BUILD_DIR)/initramfs_init_static
	@rm -rf $(BUILD_DIR)/initramfs_root
	@mkdir -p $(BUILD_DIR)/initramfs_root
	@cp $(BUILD_DIR)/initramfs_init_static $(BUILD_DIR)/initramfs_root/init
	@cp $(BUILD_DIR)/calc_os_static $(BUILD_DIR)/initramfs_root/calc_os
	@cd $(BUILD_DIR)/initramfs_root && find . -print0 | cpio --null -ov --format=newc | gzip -9 > ../initramfs.cpio.gz

qemu-initramfs: $(BUILD_DIR)/initramfs.cpio.gz
	@echo built $(BUILD_DIR)/initramfs.cpio.gz

qemu-run: $(BUILD_DIR)/initramfs.cpio.gz
	@KVER=$$(uname -r); \
	KERNEL=/boot/vmlinuz-$$KVER; \
	if [ ! -f $$KERNEL ]; then echo "missing $$KERNEL"; exit 1; fi; \
	qemu-system-x86_64 \
		-m 256M \
		-kernel $$KERNEL \
		-initrd $(BUILD_DIR)/initramfs.cpio.gz \
		-append "console=ttyS0 rdinit=/init" \
		-nographic

$(BUILD_DIR)/iso_root/boot/grub/grub.cfg:
	@mkdir -p $(BUILD_DIR)/iso_root/boot/grub
	@printf '%s\n' \
		'set default=0' \
		'set timeout=0' \
		'' \
		'serial --unit=0 --speed=115200' \
		'terminal_input serial' \
		'terminal_output serial' \
		'' \
		'menuentry "Calculator OS" {' \
		'  linux /boot/vmlinuz console=tty0 console=ttyS0,115200 rdinit=/init' \
		'  initrd /boot/initramfs.cpio.gz' \
		'}' \
		> $@

$(BUILD_DIR)/iso_root/boot/vmlinuz: $(KERNEL_IMAGE)
	@mkdir -p $(dir $@)
	@cp $(KERNEL_IMAGE) $@

$(BUILD_DIR)/iso_root/boot/initramfs.cpio.gz: $(BUILD_DIR)/initramfs.cpio.gz
	@mkdir -p $(dir $@)
	@cp $(BUILD_DIR)/initramfs.cpio.gz $@

$(BUILD_DIR)/calc_os.iso: $(BUILD_DIR)/iso_root/boot/grub/grub.cfg $(BUILD_DIR)/iso_root/boot/vmlinuz $(BUILD_DIR)/iso_root/boot/initramfs.cpio.gz
	@mkdir -p $(dir $@)
	@command -v $(GRUB_MKRESCUE) >/dev/null 2>&1 || { echo "missing $(GRUB_MKRESCUE) (install grub tools, e.g. grub-pc-bin/grub-common)"; exit 1; }
	@command -v xorriso >/dev/null 2>&1 || { echo "missing xorriso (needed by grub-mkrescue)"; exit 1; }
	@command -v mformat >/dev/null 2>&1 || { echo "missing mformat (install mtools; needed by grub-mkrescue for EFI image)"; exit 1; }
	@$(GRUB_MKRESCUE) -o $@ $(BUILD_DIR)/iso_root >/dev/null
	@echo built $@

iso: $(BUILD_DIR)/calc_os.iso

iso-run: $(BUILD_DIR)/calc_os.iso
	qemu-system-x86_64 \
		-m 256M \
		-cdrom $(BUILD_DIR)/calc_os.iso \
		-boot d \
		-serial mon:stdio \
		-display none

$(RPI_BOOT_DIR)/config.txt:
	@mkdir -p $(RPI_BOOT_DIR)
	@printf '%s\n' \
		'# Calculator OS (initramfs) - Raspberry Pi boot config' \
		'enable_uart=1' \
		'dtoverlay=disable-bt' \
		'arm_64bit=1' \
		'kernel=kernel8.img' \
		'initramfs initramfs.cpio.gz followkernel' \
		'disable_splash=1' \
		> $@

$(RPI_BOOT_DIR)/cmdline.txt:
	@mkdir -p $(RPI_BOOT_DIR)
	@printf '%s\n' \
		'console=ttyAMA0,115200 console=tty1 rdinit=/init loglevel=4' \
		> $@

$(RPI_BOOT_DIR)/kernel8.img: $(RPI_KERNEL_IMAGE)
	@mkdir -p $(RPI_BOOT_DIR)
	@if [ ! -f $(RPI_KERNEL_IMAGE) ]; then echo "missing RPI_KERNEL_IMAGE=$(RPI_KERNEL_IMAGE)"; exit 1; fi
	@cp $(RPI_KERNEL_IMAGE) $@

$(RPI_BOOT_DIR)/$(RPI_DTB):
	@mkdir -p $(RPI_BOOT_DIR)
	@if [ ! -f $(RPI_FIRMWARE_DIR)/$(RPI_DTB) ]; then echo "missing RPI_DTB=$(RPI_DTB) in RPI_FIRMWARE_DIR=$(RPI_FIRMWARE_DIR)"; exit 1; fi
	@cp $(RPI_FIRMWARE_DIR)/$(RPI_DTB) $(RPI_BOOT_DIR)/

$(RPI_BOOT_DIR)/initramfs.cpio.gz: $(BUILD_DIR)/initramfs.cpio.gz
	@mkdir -p $(RPI_BOOT_DIR)
	@cp $(BUILD_DIR)/initramfs.cpio.gz $@

$(RPI_BOOT_DIR)/start.elf:
	@mkdir -p $(RPI_BOOT_DIR)
	@if [ ! -f $(RPI_FIRMWARE_DIR)/start.elf ]; then echo "missing $(RPI_FIRMWARE_DIR)/start.elf (install raspi-firmware or set RPI_FIRMWARE_DIR)"; exit 1; fi
	@cp $(RPI_FIRMWARE_DIR)/start*.elf $(RPI_BOOT_DIR)/
	@cp $(RPI_FIRMWARE_DIR)/fixup*.dat $(RPI_BOOT_DIR)/
	@if [ -f $(RPI_FIRMWARE_DIR)/bootcode.bin ]; then cp $(RPI_FIRMWARE_DIR)/bootcode.bin $(RPI_BOOT_DIR)/; fi

rpi-boot: $(RPI_BOOT_DIR)/start.elf $(RPI_BOOT_DIR)/kernel8.img $(RPI_BOOT_DIR)/$(RPI_DTB) $(RPI_BOOT_DIR)/config.txt $(RPI_BOOT_DIR)/cmdline.txt $(RPI_BOOT_DIR)/initramfs.cpio.gz
	@echo built $(RPI_BOOT_DIR)

$(BUILD_DIR)/calc_os_rpi_boot.tar.gz: rpi-boot
	@tar -C $(RPI_BOOT_DIR) -czf $@ .
	@echo built $@

rpi-boot-tar: $(BUILD_DIR)/calc_os_rpi_boot.tar.gz

test: $(BUILD_DIR)/test_runner
	$(BUILD_DIR)/test_runner

clean:
	rm -rf $(BUILD_DIR)
