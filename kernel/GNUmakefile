# Nuke built-in rules and variables.
override MAKEFLAGS += -rR

# This is the name that our final kernel executable will have.
# Change as needed.
override KERNEL := kernel

# Convenience macro to reliably declare user overridable variables.
define DEFAULT_VAR =
    ifeq ($(origin $1),default)
        override $(1) := $(2)
    endif
    ifeq ($(origin $1),undefined)
        override $(1) := $(2)
    endif
endef

# Target architecture to build for. Default to x86_64.
override DEFAULT_ARCH := x86_64
$(eval $(call DEFAULT_VAR,ARCH,$(DEFAULT_ARCH)))

# It is suggested to use a custom built cross toolchain to build a kernel.
# We are using the standard "cc" here, it may work by using
# the host system's toolchain, but this is not guaranteed.
override DEFAULT_CC := clang
$(eval $(call DEFAULT_VAR,CC,$(DEFAULT_CC)))

override CC_IS_CLANG := no
ifeq ($(shell $(CC) --version 2>&1 | grep -i 'clang' >/dev/null 2>&1 && echo 1),1)
    override CC_IS_CLANG := yes
endif

# Same thing for "ld" (the linker).
override DEFAULT_LD := ld
$(eval $(call DEFAULT_VAR,LD,$(DEFAULT_LD)))

override LD_IS_LLD := no
ifeq ($(shell $(LD) --version 2>&1 | grep -i 'LLD' >/dev/null 2>&1 && echo 1),1)
    override LD_IS_LLD := yes
endif

COMMON_CFLAGS = -std=gnu17 -g3 -pipe -Wcast-align -Wcast-qual \
                -Wsign-conversion -Werror -Wfloat-equal -Wredundant-decls \
                -Wshadow -Wstrict-overflow=5 -Wundef -Wnull-dereference \
                -funsigned-char -Wformat=2 -Isrc/arch/$(ARCH) -DBUILD_KERNEL \
                -Wno-address-of-packed-member -fno-omit-frame-pointer \
                -fms-extensions -DFLANTERM_FB_SUPPORT_BPP

ifeq ($(ARCH), riscv64)
    override COMMON_CFLAGS += -D__riscv64
endif

ifneq ($(ARCH), x86_64)
    override COMMON_CFLAGS += -DHAS_64B_PORTS
endif

ifeq ($(IN_QEMU), 1)
    override COMMON_CFLAGS += -DIN_QEMU
endif

ifeq ($(RELEASE), 1)
    override COMMON_CFLAGS += -Ofast
endif

ifeq ($(USE_16K_PAGES), 1)
    override COMMON_CFLAGS += -DAARCH64_USE_16K_PAGES
endif

ifeq ($(DISABLE_FLANTERM), 1)
    override COMMON_CFLAGS += -DDISABLE_FLANTERM
endif

# User controllable C flags.
override DEFAULT_CFLAGS += $(COMMON_CFLAGS)
$(eval $(call DEFAULT_VAR,CFLAGS,$(DEFAULT_CFLAGS)))

# User controllable C preprocessor flags. We set none by default.
override DEFAULT_CPPFLAGS := -I..
$(eval $(call DEFAULT_VAR,CPPFLAGS,$(DEFAULT_CPPFLAGS)))

ifeq ($(ARCH), x86_64)
    # User controllable nasm flags.
    override DEFAULT_NASMFLAGS := -F dwarf -g
    $(eval $(call DEFAULT_VAR,NASMFLAGS,$(DEFAULT_NASMFLAGS)))
endif

# User controllable linker flags. We set none by default.
override DEFAULT_LDFLAGS :=
$(eval $(call DEFAULT_VAR,LDFLAGS,$(DEFAULT_LDFLAGS)))

# Internal C flags that should not be changed by the user.
override CFLAGS += \
    -Wall \
    -Wextra \
    -std=gnu11 \
    -ffreestanding \
    -fno-stack-protector \
    -fno-stack-check \
    -fno-lto \
    -fPIE

# Internal C preprocessor flags that should not be changed by the user.
override CPPFLAGS := \
    -I src \
    $(CPPFLAGS) \
    -MMD \
    -MP

ifeq ($(ARCH),x86_64)
    # Internal nasm flags that should not be changed by the user.
    override NASMFLAGS += \
        -Wall
endif

ifeq ($(DEBUG), 0)
    ifeq ($(CC_IS_CLANG),yes)
        override CFLAGS += \
            -mllvm -enable-cond-stores-vec \
            -mllvm -enable-loopinterchange \
            -mllvm -enable-loop-distribute \
            -mllvm -enable-unroll-and-jam \
            -mllvm -enable-loop-flatten \
            -mllvm -interleave-small-loop-scalar-reduction \
            -mllvm -unroll-runtime-multi-exit \
            -mllvm -aggressive-ext-opt
    endif
    ifeq ($(LD_IS_LLD),yes)
        override LDFLAGS +=
            -flto=thin \
            -Wl,--lto-O3 \
            -Wl,--lto-CGO3 \
            -Wl,-mllvm,-enable-gvn-hoist \
            -Wl,-mllvm,-loop-rotate-multi \
            -Wl,-mllvm,-polly \
            -Wl,-mllvm,-polly-invariant-load-hoisting
    endif
endif

# Architecture specific internal flags.
ifeq ($(ARCH),x86_64)
    ifeq ($(CC_IS_CLANG),yes)
        override CC += \
            -target x86_64-elf
    endif
    override CFLAGS += \
        -m64 \
        -march=x86-64-v3 \
        -mno-80387 \
        -mno-mmx \
        -mno-sse \
        -mno-sse2 \
        -mno-red-zone
    override LDFLAGS += \
        -m elf_x86_64
    override NASMFLAGS += \
        -f elf64
else ifeq ($(ARCH),aarch64)
    ifeq ($(CC_IS_CLANG),yes)
        override CC += \
            -target aarch64-elf
    else
        override CFLAGS += \
            -mno-outline-atomics
    endif
    override CFLAGS += \
        -march=armv9-a \
        -mgeneral-regs-only
    override LDFLAGS += \
        -m aarch64elf
else ifeq ($(ARCH),riscv64)
    ifeq ($(CC_IS_CLANG),yes)
        override CC += \
            -target riscv64-elf
        override CFLAGS += \
            -march=rv64imaczicboz
    else
        override CFLAGS += \
            -march=rv64imac_zicsr_zifencei
    endif
    override CFLAGS += \
        -mabi=lp64 \
        -mno-relax
    override LDFLAGS += \
        -m elf64lriscv \
        --no-relax
else
    $(error Architecture $(ARCH) not supported)
endif

# Internal linker flags that should not be changed by the user.
override LDFLAGS += \
    -nostdlib \
    -static \
    -pie \
    --no-dynamic-linker \
    -z text \
    -z max-page-size=0x1000 \
    -T src/arch/$(ARCH)/linker.ld

define FIND_FILES
    override $(1) += $(addprefix src/,$(shell cd src && find -L * -type f -name $(2) -not -path "arch/*"))
    override $(1) += $(addprefix src/arch/$(ARCH)/,$(shell cd src/arch/$(ARCH) && find -L * -type f -name $(2)))
endef

# Use "find" to glob all *.c, *.S, and *.asm files in the tree and obtain the
# object and header dependency file names.

$(eval $(call FIND_FILES,CFILES, "*.c"))
$(eval $(call FIND_FILES,ASFILES, "*.S"))
$(eval $(call FIND_FILES,NASMFILES, "*.asm"))

LIBFILES += \
    ../lib/convert.c ../lib/ctype.c ../lib/parse_printf.c ../lib/align.c \
    ../lib/format.c ../lib/adt/string_view.c ../lib/adt/range.c ../lib/util.c \
    ../lib/refcount.c ../lib/adt/string.c ../lib/time.c \
    ../lib/parse_strftime.c ../lib/adt/mutable_buffer.c \
    ../lib/adt/growable_buffer.c ../lib/string.c ../lib/adt/avltree.c \
    ../lib/adt/array.c ../lib/math.c ../lib/adt/bitmap.c ../lib/bits.c \
    ../lib/memory.c ../lib/adt/addrspace.c ../lib/size.c ../lib/adt/hashmap.c \
    ../lib/freq.c

override OBJ := $(addprefix obj/,$(CFILES:src/%.c=%.c.o) $(ASFILES:src/%.S=%.S.o) $(LIBFILES:../lib/%.c=lib/%.c.o) $(NASMFILES:src/%.asm=%.asm.o))
override HEADER_DEPS := $(addprefix obj/,$(CFILES:.c=.c.d) $(ASFILES:.S=.S.d))

# Default target.
.PHONY: all
all: bin/$(KERNEL)

src/limine.h:
	curl -Lo $@ https://github.com/limine-bootloader/limine/raw/trunk/limine.h

src/dev/pci/vendors.h:
	curl -Lo src/dev/pci/pci.ids https://pci-ids.ucw.cz/v2.2/pci.ids
	awk '/^[^#[:blank:]C]/ { num=""$$1; name=$$0; sub("^"num, "", name); sub("[[:blank:]]+", "", name); print "PCI_VENDOR(0x"num",\""name"\")" }' src/dev/pci/pci.ids > $@
	sed -i -e 's/(???)//g' $@
	sed -i -e s/\"NICEVT\"/\'NICEVT\'/g $@
	rm src/dev/pci/pci.ids
	rm $@-e

# Link rules for the final kernel executable.
bin/$(KERNEL): GNUmakefile src/arch/$(ARCH)/linker.ld $(OBJ)
	mkdir -p "$$(dirname $@)"
	$(LD) $(OBJ) $(LDFLAGS) -o $@

# Include header dependencies.
-include $(HEADER_DEPS)

# Compilation rules for *.c files.
obj/%.c.o: src/%.c GNUmakefile src/limine.h src/dev/pci/vendors.h
	mkdir -p "$$(dirname $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

obj/flanterm/%.c.o: src/flanterm/%.c GNUmakefile
	mkdir -p "$$(dirname $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -Ofast -c $< -o $@

obj/lib/%.c.o: ../lib/%.c GNUmakefile
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Compilation rules for *.S files.
obj/%.S.o: src/%.S GNUmakefile
	mkdir -p "$$(dirname $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Compilation rules for *.asm (nasm) files.
obj/%.asm.o: src/%.asm GNUmakefile
	mkdir -p "$$(dirname $@)"
	nasm $(NASMFLAGS) $< -o $@

# Remove object files and the final executable.
.PHONY: clean
clean:
	rm -rf bin obj

.PHONY: distclean
distclean: clean
	rm -f src/limine.h
	rm -f src/dev/pci/vendors.h
