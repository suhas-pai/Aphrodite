# Nuke built-in rules and variables.
MAKEFLAGS += -rR
.SUFFIXES:

# This is the name that our final kernel executable will have.
# Change as needed.
override KERNEL := kernel

# Target architecture to build for. Default to x86_64.
ARCH := x86_64

# Install prefix; /usr/local is a good, standard default pick.
PREFIX := /usr/local

# Check if the architecture is supported.
ifeq ($(filter $(ARCH),aarch64 loongarch64 riscv64 x86_64),)
    $(error Architecture $(ARCH) not supported)
endif

# User controllable C compiler command.
CC := cc

# User controllable archiver command.
AR := ar

# Same thing for "ld" (the linker).
LD := ld

override LD_IS_LLD := no
ifeq ($(shell $(LD) --version 2>&1 | grep -i 'LLD' >/dev/null 2>&1 && echo 1),1)
	override LD_IS_LLD := yes
endif

COMMON_CFLAGS = -g3 -pipe -Wcast-align -Wcast-qual -Wsign-conversion -Werror \
				 -Wfloat-equal -Wredundant-decls -Wshadow -Wstrict-overflow=5 \
				 -Wundef -Wnull-dereference -funsigned-char -Wformat=2 \
				 -Isrc/arch/$(ARCH) -DBUILD_KERNEL -Wno-microsoft \
				 -Wno-address-of-packed-member -fno-omit-frame-pointer \
				 -fms-extensions -DFLANTERM_FB_SUPPORT_BPP -DSCHED_BASIC \
				 -fsanitize=undefined

OUR_CFLAGS = -Wformat-nonliteral  -Wformat-security  -Wformat-y2k

ifeq ($(ARCH), riscv64)
	override COMMON_CFLAGS += -D__riscv64
endif

ifeq ($(ARCH), loongarch64)
	override COMMON_CFLAGS += -D__loongarch64
endif

ifneq ($(ARCH), x86_64)
	override COMMON_CFLAGS += -DHAS_64B_PORTS
endif

ifeq ($(IN_QEMU), 1)
	override COMMON_CFLAGS += -DIN_QEMU
endif

ifeq ($(RELEASE), 1)
	override COMMON_CFLAGS += -O3
else
	override COMMON_CFLAGS += -O0
endif

ifeq ($(CONFIG_16K_PAGES), 1)
	override COMMON_CFLAGS += -DAARCH64_CONFIG_16K_PAGES
endif

ifeq ($(DISABLE_FLANTERM), 1)
	override COMMON_CFLAGS += -DDISABLE_FLANTERM
endif

ifeq ($(DEBUG), 1)
	override COMMON_CFLAGS += -DDEBUG=1
endif

ifeq ($(DEBUG_LOCKS), 1)
	override COMMON_CFLAGS += -DDEBUG_LOCKS
endif

ifeq ($(CHECK_SLABS), 1)
	override COMMON_CFLAGS += -DCHECK_SLABS
endif

ifeq ($(CONFIG_UACPI), 1)
	override COMMON_CFLAGS += -DCONFIG_UACPI \
		-DUACPI_FORMATTED_LOGGING -DUACPI_NATIVE_ALLOC_ZEROED \
		-DUACPI_OVERRIDE_LIBC
endif

# User controllable C flags.
EXTRA_CFLAGS := $(COMMON_CFLAGS)

# User controllable C preprocessor flags. We set none by default.
CPPFLAGS :=

ifeq ($(ARCH), x86_64)
# User controllable nasm flags.
NASMFLAGS := -F dwarf -g
endif

# User controllable linker flags. We set none by default.
LDFLAGS :=

# Ensure the dependencies have been obtained.
ifneq ($(shell ( test '$(MAKECMDGOALS)' = clean || test '$(MAKECMDGOALS)' = distclean ); echo $$?),0)
    ifeq ($(shell ( ! test -d freestnd-c-hdrs-0bsd || ! test -d cc-runtime || ! test -f include/limine.h ); echo $$?),0)
        $(error Please run the ./get-deps script first)
    endif
endif

# Check if CC is Clang.
override CC_IS_CLANG := $(shell ! $(CC) --version | grep 'clang' >/dev/null 2>&1; echo $$?)

# Internal C flags that should not be changed by the user.
override CFLAGS += \
	-Wall \
	-Wextra \
	-std=gnu23 \
	-nostdinc \
	-ffreestanding \
	-fno-stack-protector \
	-fno-stack-check \
	-fno-lto \
	-fPIE \
	-ffunction-sections \
	-fdata-sections \
	$(EXTRA_CFLAGS)

# Internal C preprocessor flags that should not be changed by the user.
override CPPFLAGS := \
	-I include \
	-I include/arch/$(ARCH) \
	-isystem src/extern \
	-isystem freestnd-c-hdrs-0bsd \
	$(CPPFLAGS) \
	-DLIMINE_API_REVISION=2 \
	-MMD \
	-MP

ifeq ($(ARCH),x86_64)
	# Internal nasm flags that should not be changed by the user.
	override NASMFLAGS += \
		-Wall
endif

ifeq ($(RELEASE), 1)
	ifeq ($(CC_IS_CLANG),1)
		override CFLAGS += \
			-mllvm -enable-cond-stores-vec \
			-mllvm -enable-loopinterchange \
			-mllvm -enable-loop-distribute \
			-mllvm -enable-unroll-and-jam \
			-mllvm -enable-loop-flatten \
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
	ifeq ($(CC_IS_CLANG),1)
		override CC += \
			-target x86_64-unknown-none
	endif
	override CFLAGS += \
		-m64 \
		-march=x86-64-v4 \
		-mno-80387 \
		-mno-mmx \
		-mno-sse \
		-mno-sse2 \
		-mno-red-zone
	override LDFLAGS += \
		-m elf_x86_64
	override NASMFLAGS += \
		-f elf64
endif

ifeq ($(ARCH),aarch64)
	ifeq ($(CC_IS_CLANG),1)
		override CC += \
			-target aarch64-unknown-none
	else
		override CFLAGS += \
			-mno-outline-atomics
	endif
	override CFLAGS += \
		-march=armv9.3-a \
		-mgeneral-regs-only
	override LDFLAGS += \
		-m aarch64elf
endif

ifeq ($(ARCH),riscv64)
	ifeq ($(CC_IS_CLANG),1)
		override CC += \
			-target riscv64-unknown-none
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
endif

ifeq ($(ARCH),loongarch64)
    ifeq ($(CC_IS_CLANG),1)
        override CC += \
            -target loongarch64-unknown-none
    endif
    override CFLAGS += \
        -march=loongarch64 \
        -mabi=lp64s
    override LDFLAGS += \
        -m elf64loongarch \
        --no-relax
endif

# Internal linker flags that should not be changed by the user.
override LDFLAGS += \
	--build-id=none \
	-nostdlib \
	-pie \
	-z text \
	-z max-page-size=0x1000 \
	-gc-sections \
	-T src/arch/$(ARCH)/linker.ld

SRCFILES := $(shell find -L src -type f -not -path "src/arch/*" -not -path "*/.*" | LC_ALL=C sort)
SRCFILES += $(shell find -L src/arch/$(ARCH) -type f | LC_ALL=C sort)

override CFILES := $(filter %.c,$(SRCFILES))
override ASFILES := $(filter %.S,$(SRCFILES))
ifeq ($(ARCH),x86_64)
override NASMFILES := $(filter %.asm,$(SRCFILES))
endif

override OBJ := $(addprefix obj-$(ARCH)/,$(CFILES:src/%.c=%.c.o) $(ASFILES:src/%.S=%.S.o) $(NASMFILES:src/%.asm=%.asm.o))
override HEADER_DEPS := $(addprefix obj-$(ARCH)/,$(CFILES:.c=.c.d) $(ASFILES:.S=.S.d))

# Default target. This must come first, before header dependencies.
.PHONY: all
all: bin-$(ARCH)/$(KERNEL)

# Include header dependencies.
-include $(HEADER_DEPS)

# Link rules for building the C compiler runtime.
cc-runtime-$(ARCH)/cc-runtime.a: GNUmakefile cc-runtime/*
	rm -rf cc-runtime-$(ARCH)
	cp -r cc-runtime cc-runtime-$(ARCH)
	$(MAKE) -C cc-runtime-$(ARCH) -f cc-runtime.mk \
		CC="$(CC)" \
		AR="$(AR)" \
		CFLAGS="$(filter-out -Wsign-conversion -Wundef,$(CFLAGS))" \
		CPPFLAGS='-isystem ../freestnd-c-hdrs-0bsd -DCC_RUNTIME_NO_FLOAT'

include/dev/pci/vendors.h:
	curl -Lo src/dev/pci/pci.ids https://pci-ids.ucw.cz/v2.2/pci.ids
	awk '/^[^#[:blank:]C]/ { num=""$$1; name=$$0; sub("^"num, "", name); sub("[[:blank:]]+", "", name); print "PCI_VENDOR(0x"num",\""name"\")" }' src/dev/pci/pci.ids > $@
	sed -i -e 's/(???)//g' $@
	sed -i -e s/\"NICEVT\"/\'NICEVT\'/g $@
	rm src/dev/pci/pci.ids
	rm -f $@-e

# Link rules for the final kernel executable.
# The magic printf/dd command is used to force the final ELF file type to ET_DYN.
# GNU binutils, for silly reasons, forces the ELF type to ET_EXEC even for
# relocatable PIEs, if the base load address is non-0.
# See https://sourceware.org/bugzilla/show_bug.cgi?id=31795 for more information.
bin-$(ARCH)/$(KERNEL): GNUmakefile src/arch/$(ARCH)/linker.ld $(OBJ) cc-runtime-$(ARCH)/cc-runtime.a
	@mkdir -p "$$(dirname $@)"
	$(LD) $(OBJ) cc-runtime-$(ARCH)/cc-runtime.a $(LDFLAGS) -o $@
	printf '\003' | dd of=$@ bs=1 count=1 seek=16 conv=notrunc

# Compilation rules for *.c files.
obj-$(ARCH)/%.c.o: src/%.c GNUmakefile include/dev/pci/vendors.h
	@mkdir -p "$$(dirname $@)"
	$(CC) $(CFLAGS) $(OUR_CFLAGS) $(CPPFLAGS) -c $< -o $@

obj-$(ARCH)/cc-runtime.c.o: src/cc-runtime.c GNUmakefile
	@mkdir -p "$$(dirname $@)"
	$(CC) $(filter-out -Wsign-conversion -Wundef,$(CFLAGS)) $(CPPFLAGS) -c $< -o $@

obj-$(ARCH)/extern/flanterm/%.c.o: src/extern/flanterm/%.c GNUmakefile
	@mkdir -p "$$(dirname $@)"
	$(CC) $(filter-out "-Wsign-conversion -fsanitize=undefined",$(CFLAGS)) $(CPPFLAGS) -c $< -o $@

obj-$(ARCH)/extern/uacpi/%.c.o: src/extern/uacpi/%.c GNUmakefile
	@mkdir -p "$$(dirname $@)"
	$(CC) $(filter-out -Wsign-conversion -Wcast-align -Wcast-qual -Wshadow -fsanitize=undefined,$(CFLAGS)) -Iinclude/sys/uacpi $(CPPFLAGS) -c $< -o $@

obj-$(ARCH)/extern/libfdt/%.c.o: src/extern/libfdt/%.c GNUmakefile
	@mkdir -p "$$(dirname $@)"
	$(CC) $(filter-out -Wsign-conversion -Wcast-align -fsanitize=undefined,$(CFLAGS)) -Wno-unused-parameter -isystem src/extern/libfdt $(CPPFLAGS) -c $< -o $@

obj-$(ARCH)/extern/lib/%.c.o: src/extern/lib/%.c GNUmakefile
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OUR_CFLAGS) $(CPPFLAGS) -c $< -o $@

# Compilation rules for *.S files.
obj-$(ARCH)/%.S.o: src/%.S GNUmakefile
	mkdir -p "$$(dirname $@)"
	$(CC) $(CFLAGS) $(OUR_CFLAGS) $(CPPFLAGS) -c $< -o $@

# Compilation rules for *.asm (nasm) files.
obj-$(ARCH)/%.asm.o: src/%.asm GNUmakefile
	mkdir -p "$$(dirname $@)"
	nasm -felf64 $(˚NASMFLAGS) $< -o $@

# Remove object files and the final executable.
.PHONY: clean
clean:
	rm -rf bin-$(ARCH) obj-$(ARCH) cc-runtime-$(ARCH)

.PHONY: distclean
distclean:
	rm -rf bin-* obj-* freestnd-c-hdrs-0bsd cc-runtime* src/limine.h

.PHONY: install
install: all
	install -d "$(DESTDIR)$(PREFIX)/share/$(KERNEL)"
	install -m 644 bin-$(ARCH)/$(KERNEL) "$(DESTDIR)$(PREFIX)/share/$(KERNEL)/$(KERNEL)-$(ARCH)"

print:
	@echo $(CFILES)