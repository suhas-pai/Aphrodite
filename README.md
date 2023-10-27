# operating-system

An operating system written in C17 (gnu17), for x86_64, aarch64, and riscv64.

## Features

The kernel has the following implemented:
* UART drivers; uart8250 on x86_64, riscv64 and pl011 on aarch64
* Physical Memory Buddy Allocator using a vmemmap of `struct page`
* General memory allocation with kmalloc() using a slab allocator
* RTC (google,goldfish-rtc on riscv64) and LAPIC Timer, HPET on x86_64
* Keyboard (ps2) driver
* Parsing ACPI Tables and Flattened Device Tree (when available)
* Local APIC and I/O APIC
* PCI(e) Device Initialization with drivers

The following is currently being worked on.
* AHCI, SATA, IDE controllers to read disks
* VirtIO drivers
* Virtual File System (VFS)
* Full SMP support to run multiple cores
* User Processes and threads, with a scheduler to run them

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

This project can be built using the host GCC toolchain on most Linux distros on x86_64 when targetting x86_64 (`ARCH=x86_64`), but it is recommended to set up an `*-elf` [cross compiler](https://wiki.osdev.org/GCC_Cross-Compiler) for the chosen target, or use a native cross toolchain such as Clang/LLVM.

The system currently supports the `x86_64`, `aarch64` (`arm64`), and `riscv64` architectures.
`x86_64` is the most feature complete, while `arm64` and `riscv64` are catching up

Several variables are available to configure launching in QEMU:
  * `MEM` to configure memory size (in QEMU units) (By Default `4G`)
  * `SMP` to configure amount of cpus (By Default `4`)
  * `DEBUG=1` to allow starting in QEMU's debugger mode (By default `0`)
  * `CONSOLE=1` to allow starting in QEMU's console mode (By default `0`)

The all target (the default one) builds the system for the given architecture, creating a bootable .iso image containing the built system.
The all-hdd target builds a flat hard disk/USB image instead.

Here's an example command:

```make clean && make run ARCH=<arch> MEM=<mem> SMP=<smp>```
