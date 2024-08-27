# Nuke built-in rules and variables.
MAKEFLAGS += -rR
.SUFFIXES:

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
override DEFAULT_KARCH := x86_64
$(eval $(call DEFAULT_VAR,KARCH,$(DEFAULT_KARCH)))

# It is suggested to use a custom built cross toolchain to build a kernel.
# We are using the standard "cc" here, it may work by using
# the host system's toolchain, but this is not guaranteed.
override DEFAULT_KCC := clang
$(eval $(call DEFAULT_VAR,KCC,$(DEFAULT_KCC)))

override KCC_IS_CLANG := no
ifeq ($(shell $(KCC) --version 2>&1 | grep -i 'clang' >/dev/null 2>&1 && echo 1),1)
	override KCC_IS_CLANG := yes
endif

# Same thing for "ld" (the linker).
override DEFAULT_KLD := ld.lld
$(eval $(call DEFAULT_VAR,KLD,$(DEFAULT_KLD)))

override KLD_IS_LLD := no
ifeq ($(shell $(KLD) --version 2>&1 | grep -i 'LLD' >/dev/null 2>&1 && echo 1),1)
	override KLD_IS_LLD := yes
endif

COMMON_KCFLAGS = -g3 -pipe -Wcast-align -Wcast-qual -Wsign-conversion -Werror \
				 -Wfloat-equal -Wredundant-decls -Wshadow -Wstrict-overflow=5 \
				 -Wundef -Wnull-dereference -funsigned-char -Wformat=2 \
				 -Isrc/arch/$(KARCH) -DBUILD_KERNEL -Wno-microsoft \
				 -Wno-address-of-packed-member -fno-omit-frame-pointer \
				 -fms-extensions -DFLANTERM_FB_SUPPORT_BPP -DSCHED_BASIC \
				 -fsanitize=undefined

ifeq ($(KARCH), riscv64)
	override COMMON_KCFLAGS += -D__riscv64
else ifeq ($(KARCH), loongarch64)
	override COMMON_KCFLAGS += -D__loongarch64
endif

ifneq ($(KARCH), x86_64)
	override COMMON_KCFLAGS += -DHAS_64B_PORTS
endif

ifeq ($(IN_QEMU), 1)
	override COMMON_KCFLAGS += -DIN_QEMU
endif

ifeq ($(RELEASE), 1)
	override COMMON_KCFLAGS += -O3
endif

ifeq ($(USE_16K_PAGES), 1)
	override COMMON_KCFLAGS += -DAARCH64_USE_16K_PAGES
endif

ifeq ($(DISABLE_FLANTERM), 1)
	override COMMON_KCFLAGS += -DDISABLE_FLANTERM
endif

ifeq ($(DEBUG_LOCKS), 1)
	override COMMON_KCFLAGS += -DDEBUG_LOCKS
endif

ifeq ($(CHECK_SLABS), 1)
	override COMMON_KCFLAGS += -DCHECK_SLABS
endif

# User controllable C flags.
override DEFAULT_KCFLAGS += $(COMMON_KCFLAGS)
$(eval $(call DEFAULT_VAR,EXTRA_KCFLAGS,$(DEFAULT_KCFLAGS)))

# User controllable C preprocessor flags. We set none by default.
override DEFAULT_KCPPFLAGS := -I..
$(eval $(call DEFAULT_VAR,EXTRA_KCPPFLAGS,$(DEFAULT_KCPPFLAGS)))

ifeq ($(KARCH), x86_64)
# User controllable nasm flags.
override DEFAULT_KNASMFLAGS := -F dwarf -g
$(eval $(call DEFAULT_VAR,KNASMFLAGS,$(DEFAULT_KNASMFLAGS)))
endif

# User controllable linker flags. We set none by default.
override DEFAULT_KLDFLAGS :=
$(eval $(call DEFAULT_VAR,KLDFLAGS,$(DEFAULT_KLDFLAGS)))

# Internal C flags that should not be changed by the user.
override KCFLAGS += \
	-Wall \
	-Wextra \
	-std=gnu17 \
	-nostdinc \
	-ffreestanding \
	-fno-stack-protector \
	-fno-stack-check \
	-fno-lto \
	-fPIE \
	-ffunction-sections \
	-fdata-sections \
	$(EXTRA_KCFLAGS)

# Internal C preprocessor flags that should not be changed by the user.
override KCPPFLAGS := \
	-I src \
	$(EXTRA_KCPPFLAGS) \
	-isystem freestanding-headers \
	-MMD \
	-MP

ifeq ($(KARCH),x86_64)
	# Internal nasm flags that should not be changed by the user.
	override NASMFLAGS += \
		-Wall
endif

ifeq ($(RELEASE), 1)
	ifeq ($(KCC_IS_CLANG),yes)
		override KCFLAGS += \
			-mllvm -enable-cond-stores-vec \
			-mllvm -enable-loopinterchange \
			-mllvm -enable-loop-distribute \
			-mllvm -enable-unroll-and-jam \
			-mllvm -enable-loop-flatten \
			-mllvm -interleave-small-loop-scalar-reduction \
			-mllvm -unroll-runtime-multi-exit \
			-mllvm -aggressive-ext-opt
	endif
	ifeq ($(KLD_IS_LLD),yes)
		override KLDFLAGS +=
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
ifeq ($(KARCH),x86_64)
	ifeq ($(KCC_IS_CLANG),yes)
		override KCC += \
			-target x86_64-elf
	endif
	override KCFLAGS += \
		-m64 \
		-march=x86-64-v3 \
		-mno-80387 \
		-mno-mmx \
		-mno-sse \
		-mno-sse2 \
		-mno-red-zone
	override KLDFLAGS += \
		-m elf_x86_64
	override KNASMFLAGS += \
		-f elf64
else ifeq ($(KARCH),aarch64)
	ifeq ($(KCC_IS_CLANG),yes)
		override KCC += \
			-target aarch64-elf
	else
		override KCFLAGS += \
			-mno-outline-atomics
	endif
	override KCFLAGS += \
		-march=armv9.3-a \
		-mgeneral-regs-only
	override KLDFLAGS += \
		-m aarch64elf
else ifeq ($(KARCH),riscv64)
	ifeq ($(KCC_IS_CLANG),yes)
		override KCC += \
			-target riscv64-elf
		override KCFLAGS += \
			-march=rv64imaczicboz
	else
		override CFLAGS += \
			-march=rv64imac_zicsr_zifencei
	endif
	override KCFLAGS += \
		-mabi=lp64 \
		-mno-relax
	override KLDFLAGS += \
		-m elf64lriscv \
		--no-relax
else ifeq ($(KARCH),loongarch64)
    ifeq ($(KCC_IS_CLANG),yes)
        override KCC += \
            -target loongarch64-none
    endif
    override KCFLAGS += \
        -march=loongarch64 \
        -mabi=lp64s
    override KLDFLAGS += \
        -m elf64loongarch \
        --no-relax
else
	$(error Architecture $(KARCH) not supported)
endif

# Internal linker flags that should not be changed by the user.
override KLDFLAGS += \
	-nostdlib \
	-pie \
	-z text \
	-z max-page-size=0x1000 \
	-gc-sections \
	-T src/arch/$(KARCH)/linker.ld

define FIND_FILES
	override $(1) += $(addprefix src/,$(shell cd src && find -L * -type f -name $(2) -not -path "arch/*"))
	override $(1) += $(addprefix src/arch/$(KARCH)/,$(shell cd src/arch/$(KARCH) && find -L * -type f -name $(2)))
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

override OBJ := $(addprefix obj-$(KARCH)/,$(CFILES:src/%.c=%.c.o) $(ASFILES:src/%.S=%.S.o) $(LIBFILES:../lib/%.c=lib/%.c.o) $(NASMFILES:src/%.asm=%.asm.o))
override HEADER_DEPS := $(addprefix obj-$(KARCH)/,$(CFILES:.c=.c.d) $(ASFILES:.S=.S.d))

# Ensure the dependencies have been obtained.
override MISSING_DEPS := $(shell if ! test -d freestanding-headers || ! test -f src/cc-runtime.c || ! test -f src/limine.h || ! test -f freestanding-headers/stdatomic.h; then echo 1; fi)
ifeq ($(MISSING_DEPS),1)
    $(error Please run the ./get-deps script first)
endif

# Default target.
.PHONY: all
all: bin-$(KARCH)/$(KERNEL)

src/dev/pci/vendors.h:
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
bin-$(KARCH)/$(KERNEL): GNUmakefile src/arch/$(KARCH)/linker.ld $(OBJ)
	mkdir -p "$$(dirname $@)"
	$(KLD) $(OBJ) $(KLDFLAGS) -o $@
	printf '\003' | dd of=$@ bs=1 count=1 seek=16 conv=notrunc

# Include header dependencies.
-include $(HEADER_DEPS)

# Compilation rules for *.c files.
obj-$(KARCH)/%.c.o: src/%.c GNUmakefile src/dev/pci/vendors.h
	mkdir -p "$$(dirname $@)"
	$(KCC) $(KCFLAGS) $(KCPPFLAGS) -c $< -o $@

obj-$(KARCH)/cc-runtime.c.o: src/cc-runtime.c GNUmakefile
	mkdir -p "$$(dirname $@)"
	$(KCC) $(filter-out -Wsign-conversion -Wundef,$(KCFLAGS)) $(KCPPFLAGS) -c $< -o $@

obj-$(KARCH)/flanterm/%.c.o: src/flanterm/%.c GNUmakefile
	mkdir -p "$$(dirname $@)"
	$(KCC) $(filter-out -fsanitize=undefined,$(KCFLAGS)) -O3 $(KCPPFLAGS) -c $< -o $@

obj-$(KARCH)/lib/%.c.o: ../lib/%.c GNUmakefile
	@mkdir -p $(dir $@)
	$(KCC) $(KCFLAGS) $(KCPPFLAGS) -c $< -o $@

# Compilation rules for *.S files.
obj-$(KARCH)/%.S.o: src/%.S GNUmakefile
	mkdir -p "$$(dirname $@)"
	$(KCC) $(KCFLAGS) $(KCPPFLAGS) -c $< -o $@

# Compilation rules for *.asm (nasm) files.
obj-$(KARCH)/%.asm.o: src/%.asm GNUmakefile
	mkdir -p "$$(dirname $@)"
	nasm -felf64 $(˚NASMFLAGS) $< -o $@

# Remove object files and the final executable.
.PHONY: clean
clean:
	rm -rf bin-$(KARCH) obj-$(KARCH)

.PHONY: distclean
distclean:
	rm -rf bin-* obj-* freestanding-headers
	rm -f src/limine.h
	rm -f src/dev/pci/vendors.h

.PHONY: install
install: all
	install -d "$(DESTDIR)$(PREFIX)/share/$(KERNEL)"
	install -m 644 bin-$(KARCH)/$(KERNEL) "$(DESTDIR)$(PREFIX)/share/$(KERNEL)/$(KERNEL)-$(KARCH)"