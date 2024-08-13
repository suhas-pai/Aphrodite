# Aphrodite

An operating system written in C17 (gnu17), for `x86_64`, `aarch64`, and `riscv64`.
`loongarch64` is supported but isn't currently functional.

`x86_64` is the most feature complete, while `aarch64` and `riscv64` are catching up

Migrated from older repository: https://github.com/suhas-pai/operating-system-old
Check older repository for full commit history

## Features

The project has the implemented all of the following:
* Has serial UART drivers; uart8250 on x86_64, riscv64 and pl011 on aarch64
* Full abstract virtual memory manager with large page support.
* Implements `kmalloc()` using a slab allocator.
* Keeps time with RTC (`google,goldfish-rtc` on riscv64) and LAPIC Timer, HPET on x86_64
* Discovers devices in ACPI Tables and Device Tree (when available)
* Finds and Initializes PCI(e) Devices.

And more

The following is currently being thought out and worked on.
* IDE, AHCI, SATA, NVME controllers to read disks
* VirtIO drivers
* Virtual File System (VFS)
* Full SMP support with scheduler
* User processes and threads

## Building
### Prerequisites

It's recommended that LLVM/Clang be used as the toolchain and compiler, GCC may not
be able to build this project.

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

Install the latest version of LLVM available. Best option is from llvm.org.
The llvm version available in your system's package manager may be too old to
build this project.

Make sure to also add llvm's installation folder to your `$PATH` environment variable.
This project also requires installing `nasm` and `xorisso`.

#### Targets

The GNUmakefile provides the following targets to use with `make` for building:
 * `all` target builds the system for the given architecture in `KARCH`, creating
   a bootable .iso image containing the built system.
 * `all-hdd` target builds a flat hard disk/USB image instead.
 * `install` target can install the system into a directory provided by the variable `DESTDIR`.

#### Variables

Several variables are available to configure how the project is built:
 * `KARCH=` specifies the architecture the project is built for. Default is `x86_64`,
    but also accepts `aarch64`, `riscv64`, `loongarch64` as inputs to run the
    project on the respective architecture.
 * `KCC=`, `KLD=` detail the compiler and linker used to build the *kernel*.
   Default is `KCC=clang` `KLD=ld.lld`.

## Running

This project uses the Makefile build system. To run the project (In QEMU), use
the following command:

``` make run KARCH=x86_64 KCC=clang KLD=ld.lld```

To run from a built raw HDD image, rather than a bootable ISO, run the following
command instead:

``` make run-hdd KARCH=x86_64 KCC=clang KLD=ld.lld```

On `x86_64`, additional options are available:
 * `run-bios`, `run-hdd-bios` Makefile targets are equivalent to their non `-bios` counterparts
   except that they boot QEMU using the default `SeaBIOS` firmware instead of `OVMF`.

Several variables are also available to configure the system provided by QEMU.
If not provided, they are given a default value that is detailed below:

  * `MEM=` to configure memory size (in QEMU units). Default is `4G`
  * `SMP=` to configure amount of cpus. Default is `4`
  * `DEBUG=` to build in debug mode and to start in QEMU in debugger mode. Default is `0`
  * `RELEASE=` to build in release mode with full optimizations (though `ubsan` is still enabled). Default is `0`
  * `CONSOLE=` to start in QEMU's console mode. Default is `0`
  * `DISABLE_ACPI=` to start QEMU's machine without ACPI, relying exclusively on the DTB (Flattened Device Tree).
    Default is `0`
  * `DISABLE_FLANTERM=` to disable `flanterm`, the terminal emulator used. This option helps improve bootup
    performance. Default is `0`
  * `DRIVE_KIND=` to specify what drives are available from QEMU. Default is `scsi` for riscv64, and `nvme` for all other archs.
    Available options are:
     * `block` which provides the default cdrom/hda access from QEMU
     * `scsi` which provides `VirtIO` to access drives through the `virtio-scsi` interface
     * `nvme` which provides nvme drives instread
  * `NVME_MAX_QUEUE_COUNT=` to set nvme's max queue count. Only available when `DRIVE_KIND` is nvme. Default is `64`
  * `TRACE=` to trace certain logs in qemu to `log.txt`. Takes a space separated string and provides qemu
    with the list in the correct format. Default is `""`
  * `CHECK_SLABS=` to enable pervasive slab checks in `kmalloc()` and other slab allocators.
     Default is `0`
  * `DEBUG_LOCKS=` to enable pervasive lock integrity checks. Default is `0`
