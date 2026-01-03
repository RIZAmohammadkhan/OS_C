# Calculator OS

Small, robust "calculator OS" simulation written in portable C.

What it contains

- Tiny cooperative kernel: [src/kernel/kernel.c](src/kernel/kernel.c), [src/kernel/kernel.h](src/kernel/kernel.h)
- Console drivers (display/keypad): [src/drivers/console_display.c](src/drivers/console_display.c), [src/drivers/console_display.h](src/drivers/console_display.h), [src/drivers/console_keypad.c](src/drivers/console_keypad.c), [src/drivers/console_keypad.h](src/drivers/console_keypad.h)
- Scientific calculator app (REPL): [src/apps/calc_app.c](src/apps/calc_app.c), [src/apps/calc_app.h](src/apps/calc_app.h)
- Expression lexer / parser / AST evaluator / formatter: [src/calc/lexer.c](src/calc/lexer.c), [src/calc/lexer.h](src/calc/lexer.h), [src/calc/parser.c](src/calc/parser.c), [src/calc/parser.h](src/calc/parser.h), [src/calc/eval.c](src/calc/eval.c), [src/calc/eval.h](src/calc/eval.h), [src/calc/format.c](src/calc/format.c), [src/calc/format.h](src/calc/format.h), [src/calc/tokens.h](src/calc/tokens.h)
- Platform-specific code: [src/platform/linux_poweroff.c](src/platform/linux_poweroff.c), [src/platform/linux_poweroff.h](src/platform/linux_poweroff.h), [src/platform/initramfs_init.c](src/platform/initramfs_init.c)
- Utilities: [src/util/strutil.c](src/util/strutil.c), [src/util/strutil.h](src/util/strutil.h), [src/util/status.c](src/util/status.c), [src/util/status.h](src/util/status.h)
- Small test suite: [tests/test_main.c](tests/test_main.c)
- Build and run helpers: [Makefile](Makefile)

Quickstart

1. Build the program:

```bash
make
```

2. Run the interactive simulation locally (console):

```bash
make run
```

3. Run unit tests:

```bash
make test
```

Booting under QEMU (optional)

This repository includes a helper to build a static `calc_os` binary, pack it into a minimal initramfs, and boot it with your host kernel inside QEMU.

- Build the static initramfs:

```bash
make qemu-initramfs
```

- Boot interactively in QEMU (serial console):

```bash
make qemu-run
```

Bootable ISO (x86_64 PC / VM)

You can build a bootable ISO image that runs the calculator as Linux PID 1.

- Build the ISO:

```bash
make iso
```

Dependencies for `make iso`

- `grub-mkrescue` (GRUB tools)
- `xorriso`
- `mtools` (provides `mformat`)

On Debian/Ubuntu:

```bash
sudo apt-get install grub-pc-bin grub-common xorriso mtools
```

This produces `build/calc_os.iso`. By default it embeds your host kernel from `/boot/vmlinuz-$(uname -r)`.
If you want to use a different kernel image:

```bash
make iso KERNEL_IMAGE=/path/to/vmlinuz-or-bzImage
```

- Boot the ISO in QEMU (serial/terminal):

```bash
make iso-run
```

Notes

- "Any device" depends on CPU architecture and firmware. The provided ISO target is intended for x86_64 machines/VMs (typical PCs). For ARM (e.g. Raspberry Pi) or UEFI-only/secure-boot environments, tell me the target and I can adapt the build.

Raspberry Pi (ARM) boot folder

For Raspberry Pi, the simplest approach is: use the Pi firmware + a Pi kernel, and boot the calculator from an initramfs (no root filesystem needed).

1) Build the initramfs (on a Pi, or using an ARM cross-compiler):

```bash
make qemu-initramfs
```

If cross-compiling on Debian for 64-bit Pi (aarch64), you can do:

```bash
sudo apt-get install gcc-aarch64-linux-gnu libc6-dev-arm64-cross
make qemu-initramfs CC=aarch64-linux-gnu-gcc
```

2) Assemble a Pi-bootable `boot/` folder:

```bash
make rpi-boot
```

This creates `build/rpi_boot/` containing `start*.elf`, `fixup*.dat`, `kernel8.img`, a device tree blob, `config.txt`, `cmdline.txt`, and `initramfs.cpio.gz`.

By default it expects Pi firmware in `/usr/lib/raspi-firmware` and uses:

- `RPI_KERNEL_IMAGE=/usr/lib/raspi-firmware/kernel8.img`
- `RPI_DTB=bcm2711-rpi-4-b.dtb` (Pi 4B)

Override these for your specific model/kernel:

```bash
make rpi-boot RPI_FIRMWARE_DIR=/path/to/firmware RPI_KERNEL_IMAGE=/path/to/kernel8.img RPI_DTB=your-model.dtb
```

3) Copy `build/rpi_boot/` onto the SD card FAT boot partition.

After boot, the calculator runs on the serial console at 115200 baud.

Notes about QEMU run

- `make qemu-run` uses `qemu-system-x86_64` and your local kernel image `/boot/vmlinuz-$(uname -r)` together with `build/initramfs.cpio.gz` and runs `/calc_os` as `init` inside the VM. It requires `qemu-system-x86_64` and typical ISO/initramfs helpers (e.g., `grub-mkrescue`, `xorriso`) to be available on the host.
- When running under QEMU the program becomes PID 1. Typing `exit` will stop the calculator app and attempt a clean shutdown.

Commands supported by the REPL

- `help` — show available commands
- `mode deg|rad` — switch trig angle units
- `mem`, `mem set <expr>`, `mem clear` — memory register
- `ans` — last computed answer, usable in expressions
- `exit` — exit the REPL (shuts down when running as PID 1 under QEMU)

Examples

```bash
2+2*3
sin(30)
ans + 5
mem set 42
mem
```

If you want this ported to a microcontroller or custom hardware (e.g., ARM Cortex-M with an LCD/key matrix), tell me the target and I will adapt the same kernel/app structure and provide linker scripts and driver stubs.
