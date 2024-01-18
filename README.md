# Aphrodite

An operating system written in C17 (gnu17), for x86_64, aarch64, and riscv64.

Migrated from older repository: https://github.com/suhas-pai/operating-system-old
Check older repository for full commit history

## Features

The kernel has the following implemented:
* UART drivers; uart8250 on x86_64, riscv64 and pl011 on aarch64
* Physical memory buddy allocator using a vmemmap of `struct page`
* General memory allocation with kmalloc() using a slab allocator
* RTC (google,goldfish-rtc on riscv64) and LAPIC Timer, HPET on x86_64
* Keyboard (ps2) driver
* Parsing ACPI Tables and DTB (Flattened Device Tree) (when available)
* Local APIC and I/O APIC
* PCI(e) Device Initialization with drivers

The following is currently being worked on.
* AHCI, SATA, IDE controllers to read disks
* VirtIO drivers
* Virtual File System (VFS)
* Full SMP support with scheduler
* User processes and threads

In addition, the system comes with a basic standard library that includes:

* Abstract `printf()` formatter
  * Used to implement `kprintf()` and `format_to_string()`, and more
* Abstract `strftime()` formatter
  * Used to implement a very basic `kstrftime()`
* Integer Conversion APIs
  * Can convert to and from strings, with the following configurable integer formats:
    * Binary (with `0b` prefix)
    * Octal (with `0` or `0o` prefix)
    * Decimal
    * Hexadecimal (with `0x` prefix)
    * Alpbabetic (with `0a` prefix)
  * Used by `printf()` formatter for converting integers to strings
* AVL Trees
  * Used to create abstract `struct address_space` type

And more

## Building

It is recommended to build this project using a standard Linux distro, using a Clang/LLVM toolchain capable of cross compilation.

This project can be built using Clang/LLVM for all architectures.

This project can also be built using the host GCC toolchain on x86_64 when targetting x86_64 (`ARCH=x86_64`), but it is recommended to set up an `*-elf` [cross compiler](https://wiki.osdev.org/GCC_Cross-Compiler) for the chosen target, or use a native cross toolchain such as Clang/LLVM.

The system currently supports the `x86_64`, `aarch64` (`arm64`), and `riscv64` architectures.
`x86_64` is the most feature complete, while `aarch64` and `riscv64` are catching up

Several variables are available to configure launching in QEMU:
  * `MEM=` to configure memory size (in QEMU units) (By default `4G`)
  * `SMP=` to configure amount of cpus (By default `4`)
  * `DEBUG=1` to build in debug mode and to start in QEMU in debugger mode (By default `0`)
  * `RELEASE=1` to build in release mode with full optimizations (though `ubsan` is still enabled) (By default `0`)
  * `CONSOLE=1` to start in QEMU's console mode (By default `0`)
  * `DISABLE_ACPI=1` to start QEMU's machine without ACPI, relying exclusively on the DTB (Flattened Device Tree) (by default `0`)
  * `DISABLE_FLANTERM=1` to disable `flanterm`, the terminal emulator used. This option helps improve kernel performance (by default `0)
  * `TRACE=""` to trace certain logs in qemu to `log.txt`. Takes a space separated string and provides qemu with the correct argumentd (by default `""`)

The `all` target (default target) builds the system for the given architecture, creating a bootable .iso image containing the built system.
The `all-hdd` target builds a flat hard disk/USB image instead.

Here's an example command:

```make clean && make run ARCH=<arch> MEM=<mem> SMP=<smp> RELEASE=1```
