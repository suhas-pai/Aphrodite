# Aphrodite

An operating system written in C17 (gnu17), for x86_64, aarch64, and riscv64.

`loongarch64` is being worked on, and isn't currently functional.
`x86_64` is the most feature complete, while `aarch64` and `riscv64` are catching up

Migrated from older repository: https://github.com/suhas-pai/operating-system-old
Check older repository for full commit history

## Features

The project has the implemented all of the following:
* Has serial UART drivers; uart8250 on x86_64, riscv64 and pl011 on aarch64
* Full abstract virtual memory manager with large page support.
* Implements `kmalloc()` using a slab allocator.
* Keeps time with RTC (google,goldfish-rtc on riscv64) and LAPIC Timer, HPET on x86_64
* Discovers devices in ACPI Tables and Device Tree (when available)
* Finds and Initializes PCI(e) Devices.

The following is currently being worked on.
* IDE, AHCI, SATA, NVME controllers to read disks
* VirtIO drivers
* Virtual File System (VFS)
* Full SMP support with scheduler
* User processes and threads

And more

## Building
### Prerequisites

We recommended using LLVM/Clang as the toolchain and compiler, GCC may/may not be
able to build this project.

The `kernel` GitHub Workflow follows the steps detailed below on both macOS and
Ubuntu (Linux) to build this project.

#### On macOS

Run the following command to install the required packages from Homebrew.

```brew install llvm make nasm xorriso coreutils```

Per the installation instructions of llvm, make, and coreutils, add the following
paths to your `$PATH` environment variable:

```
/opt/homebrew/opt/llvm/bin
/opt/homebrew/opt/make/libexec/gnubin
/usr/local/opt/coreutils/libexec/gnubin
```
#### On Linux

Install the latest version of LLVM from their website: llvm.org. Make sure to
add llvm's installation folder to your `$PATH` environment variable.

This project also requires `nasm` and `xorisso`, which should be found in your
system's package manager.

#### Targets

The GNUmakefile provides the following targets to use with `make` for building.

The `all` target (default target) builds the system for the given architecture in `$KARCH`,
creating a bootable .iso image containing the built system.

The `all-hdd` target builds a flat hard disk/USB image instead.

The `install` target can install the system into a directory provided by variable `DESTDIR`.

## Running

This project uses the Makefile build system. To run the project (In QEMU), use
the following command:

``` make run KARCH=x86_64 KCC=clang KLD=ld.lld```

The `KARCH` variable also accepts `aarch64`, `riscv64`, `loongarch64` as inputs to run
the project on the respective architecture.

Several other variables are also available. They allow configuration of many
different parts of the system. If not provided, they are given a default value
detailed below:

  * `MEM=` to configure memory size (in QEMU units). Default is `4G`
  * `SMP=` to configure amount of cpus. Default is `4`
  * `DEBUG=` to build in debug mode and to start in QEMU in debugger mode. Default is `0`
  * `RELEASE=` to build in release mode with full optimizations (though `ubsan` is still enabled). Default is `0`
  * `CONSOLE=` to start in QEMU's console mode. Default is `0`
  * `DISABLE_ACPI=` to start QEMU's machine without ACPI, relying exclusively on the DTB (Flattened Device Tree).
    Default is `0`
  * `DISABLE_FLANTERM=` to disable `flanterm`, the terminal emulator used. This option helps improve bootup
    performance. Default is `0`
  * `TRACE=` to trace certain logs in qemu to `log.txt`. Takes a space separated string and provides qemu
    with the correct argumentd. Default is `""`
  * `CHECK_SLABS=` to enable pervasive slab checks in `kmalloc()` and other slab allocators.
     Default is `0`
