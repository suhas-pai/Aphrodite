# Nuke built-in rules and variables.
override MAKEFLAGS += -rR
export IN_QEMU := 0

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

override IMAGE_NAME := template-$(KARCH)
override DEFAULT_MEM := 4G

$(eval $(call DEFAULT_VAR,MEM,$(DEFAULT_MEM)))

override DEFAULT_SMP := 4
$(eval $(call DEFAULT_VAR,SMP,$(DEFAULT_SMP)))

EXTRA_QEMU_ARGS=-d unimp -d guest_errors -d int -D ./log.txt -rtc base=localtime
ifeq ($(DEBUG), 1)
	EXTRA_QEMU_ARGS += -s -S -serial stdio
else ifeq ($(CONSOLE), 1)
	EXTRA_QEMU_ARGS += -s -S -monitor stdio -no-reboot -no-shutdown
else
	EXTRA_QEMU_ARGS += -serial stdio
endif

DEFAULT_MACHINE=virt
ifeq ($(KARCH), x86_64)
	DEFAULT_MACHINE=q35
endif

MACHINE=$(DEFAULT_MACHINE)
ifeq ($(KARCH), x86_64)
ifeq ($(DISABLE_ACPI), 1)
$(error ACPI cannot be disabled on x86_64)
endif
else
	ifeq ($(DISABLE_ACPI), 1)
		MACHINE := $(MACHINE),acpi=off
	endif
endif

ifeq ($(KARCH), aarch64)
	MACHINE := $(MACHINE),gic-version=max
endif

DEFAULT_DRIVE_KIND=nvme
ifeq ($(KARCH), riscv64)
	DEFAULT_DRIVE_KIND = scsi
	MACHINE := $(MACHINE),aclint=on,aia=aplic-imsic,aia-guests=1
endif

DRIVE_KIND?=nvme
$(eval $(call DEFAULT_VAR,DRIVE_KIND,$(DEFAULT_DRIVE_KIND)))

DEFAULT_TRACE=
$(eval $(call DEFAULT_VAR,TRACE,$(DEFAULT_TRACE)))

ifneq ($(TRACE),)
	TRACE_LIST=$(wordlist 1, 2147483647, $(TRACE))
	EXTRA_QEMU_ARGS += $(foreach trace_var,$(TRACE_LIST),$(addprefix -trace , $(trace_var)))
endif

DEFAULT_DISABLE_FLANTERM=0
$(eval $(call DEFAULT_VAR,DISABLE_FLANTERM,$(DEFAULT_DISABLE_FLANTERM)))

DEFAULT_DEBUG_LOCKS=0
$(eval $(call DEFAULT_VAR,DEBUG_LOCKS,$(DEFAULT_DEBUG_LOCKS)))

DEFAULT_CHECK_SLABS=0
$(eval $(call DEFAULT_VAR,CHECK_SLABS,$(DEFAULT_CHECK_SLABS)))

VIRTIO_CD_QEMU_ARG=""
VIRTIO_HDD_QEMU_ARG=""

QEMU_CDROM_ARGS=\
	-drive if=pflash,unit=0,format=raw,file=ovmf/ovmf-code-$(KARCH).fd,readonly=on \
	-drive if=pflash,unit=1,format=raw,file=ovmf/ovmf-vars-$(KARCH).fd

QEMU_HDD_ARGS=\
	-drive if=pflash,unit=0,format=raw,file=ovmf/ovmf-code-$(KARCH).fd,readonly=on \
	-drive if=pflash,unit=1,format=raw,file=ovmf/ovmf-vars-$(KARCH).fd

ifeq ($(DRIVE_KIND),block)
	ifeq ($(KARCH), riscv64)
$(error "block device not supported on riscv64")
	endif

	QEMU_CDROM_ARGS += -cdrom $(IMAGE_NAME).iso
	QEMU_HDD_ARGS += -hda $(IMAGE_NAME).hdd
else ifeq ($(DRIVE_KIND), scsi)
	QEMU_CDROM_ARGS += \
		-device virtio-scsi-pci,id=scsi \
		-device scsi-cd,drive=cd0 \
		-drive id=cd0,if=none,format=raw,file=$(IMAGE_NAME).iso
	QEMU_HDD_ARGS += \
		-device virtio-scsi-pci,id=scsi \
		-device scsi-hd,drive=hd0 \
		-drive id=hd0,format=raw,file=$(IMAGE_NAME).hdd
else ifeq ($(DRIVE_KIND),nvme)
DEFAULT_NVME_MAX_QUEUE_COUNT=64
$(eval $(call DEFAULT_VAR,NVME_MAX_QUEUE_COUNT,$(DEFAULT_NVME_MAX_QUEUE_COUNT)))

	QEMU_CDROM_ARGS += \
		-drive file=$(IMAGE_NAME).iso,if=none,id=osdrive,format=raw \
		-device nvme,serial=1234,drive=osdrive,id=nvme0,max_ioqpairs=$(NVME_MAX_QUEUE_COUNT)
	QEMU_HDD_ARGS += \
		-drive file=$(IMAGE_NAME).hdd,if=none,id=osdrive,format=raw \
		-device nvme,serial=1234,drive=osdrive,id=nvme0,max_ioqpairs=$(NVME_MAX_QUEUE_COUNT)
else
$(error "Unrecognized value for variable DRIVE_KIND: \"${DRIVE_KIND}\"")
endif

# Default user QEMU flags. These are appended to the QEMU command calls.
override DEFAULT_QEMUFLAGS := -M $(MACHINE) -m $(MEM) -smp $(SMP)
$(eval $(call DEFAULT_VAR,QEMUFLAGS,$(DEFAULT_QEMUFLAGS)))

.PHONY: all
all: $(IMAGE_NAME).iso

.PHONY: all-hdd
all-hdd: $(IMAGE_NAME).hdd

.PHONY: run
run: run-$(KARCH)

.PHONY: run-hdd
run-hdd: run-hdd-$(KARCH)

.PHONY: run-x86_64
run-x86_64: ovmf/ovmf-code-$(KARCH).fd ovmf/ovmf-vars-$(KARCH).fd $(IMAGE_NAME).iso
	qemu-system-$(KARCH) \
		-cpu max \
		$(QEMUFLAGS) \
		$(QEMU_CDROM_ARGS) \
		$(EXTRA_QEMU_ARGS)

.PHONY: run-hdd-x86_64
run-hdd-x86_64: ovmf/ovmf-code-$(KARCH).fd ovmf/ovmf-vars-$(KARCH).fd $(IMAGE_NAME).hdd
	qemu-system-$(KARCH) \
		-cpu max \
		$(QEMUFLAGS) \
		$(QEMU_HDD_ARGS) \
		$(EXTRA_QEMU_ARGS)

.PHONY: run-aarch64
run-aarch64: ovmf/ovmf-code-$(KARCH).fd ovmf/ovmf-vars-$(KARCH).fd $(IMAGE_NAME).iso
	qemu-system-$(KARCH) \
		-cpu max \
		-device ramfb \
		-device qemu-xhci \
		-device usb-kbd \
		-device usb-mouse \
		$(QEMUFLAGS) \
		$(QEMU_CDROM_ARGS) \
		$(EXTRA_QEMU_ARGS)

.PHONY: run-hdd-aarch64
run-hdd-aarch64: ovmf/ovmf-code-$(KARCH).fd ovmf/ovmf-vars-$(KARCH).fd $(IMAGE_NAME).hdd
	qemu-system-$(KARCH) \
		-cpu max \
		-device ramfb \
		-device qemu-xhci \
		-device usb-kbd \
		-device usb-mouse \
		$(QEMUFLAGS) \
		$(QEMU_HDD_ARGS) \
		$(EXTRA_QEMU_ARGS)

.PHONY: run-riscv64
run-riscv64: ovmf/ovmf-code-$(KARCH).fd ovmf/ovmf-vars-$(KARCH).fd $(IMAGE_NAME).iso
	qemu-system-$(KARCH) \
		-cpu max \
		-device ramfb \
		-device qemu-xhci \
		-device usb-kbd \
		-device usb-mouse \
		$(QEMUFLAGS) \
		$(QEMU_CDROM_ARGS) \
		$(EXTRA_QEMU_ARGS)

.PHONY: run-hdd-riscv64
run-hdd-riscv64: ovmf/ovmf-code-$(KARCH).fd ovmf/ovmf-vars-$(KARCH).fd $(IMAGE_NAME).hdd
	qemu-system-$(KARCH) \
		-cpu max \
		-device ramfb \
		-device qemu-xhci \
		-device usb-kbd \
		-device usb-mouse \
		$(QEMUFLAGS) \
		$(QEMU_HDD_ARGS) \
		$(EXTRA_QEMU_ARGS)

.PHONY: run-loongarch64
run-loongarch64: ovmf/ovmf-code-$(KARCH).fd ovmf/ovmf-vars-$(KARCH).fd $(IMAGE_NAME).iso
	qemu-system-$(KARCH) \
		-device ramfb \
		-device qemu-xhci \
		-device usb-kbd \
		-device usb-mouse \
		$(QEMUFLAGS) \
		$(QEMU_CDROM_ARGS) \
		$(EXTRA_QEMU_ARGS)

.PHONY: run-hdd-loongarch64
run-hdd-loongarch64: ovmf/ovmf-code-$(KARCH).fd ovmf/ovmf-vars-$(KARCH).fd $(IMAGE_NAME).hdd
	qemu-system-$(KARCH) \
		-device ramfb \
		-device qemu-xhci \
		-device usb-kbd \
		-device usb-mouse \
		$(QEMUFLAGS) \
		$(QEMU_HDD_ARGS) \
		$(EXTRA_QEMU_ARGS)

.PHONY: run-bios
run-bios: $(IMAGE_NAME).iso
	qemu-system-x86_64 \
		-cpu max \
		-cdrom $(IMAGE_NAME).iso \
		-boot d \
		$(QEMUFLAGS) \
		$(EXTRA_QEMU_ARGS)

.PHONY: run-hdd-bios
run-hdd-bios: $(IMAGE_NAME).hdd
	qemu-system-x86_64 \
		-cpu max \
		-hda $(IMAGE_NAME).hdd \
		$(QEMUFLAGS) \
		$(EXTRA_QEMU_ARGS)

ovmf/ovmf-code-$(KARCH).fd:
	mkdir -p ovmf
	curl -Lo $@ https://github.com/limine-bootloader/edk2-ovmf-nightly/releases/latest/download/ovmf-code-$(KARCH).fd
	if [ "$(KARCH)" = "aarch64" ]; then dd if=/dev/zero of=$@ bs=1 count=0 seek=67108864 2>/dev/null; fi
	if [ "$(KARCH)" = "riscv64" ]; then dd if=/dev/zero of=$@ bs=1 count=0 seek=33554432 2>/dev/null; fi

ovmf/ovmf-vars-$(KARCH).fd:
	mkdir -p ovmf
	curl -Lo $@ https://github.com/limine-bootloader/edk2-ovmf-nightly/releases/latest/download/ovmf-vars-$(KARCH).fd
	if [ "$(KARCH)" = "aarch64" ]; then dd if=/dev/zero of=$@ bs=1 count=0 seek=67108864 2>/dev/null; fi
	if [ "$(KARCH)" = "riscv64" ]; then dd if=/dev/zero of=$@ bs=1 count=0 seek=33554432 2>/dev/null; fi

limine/limine:
	rm -rf limine
	git clone https://github.com/limine-bootloader/limine.git --branch=v8.x-binary --depth=1
	$(MAKE) -C limine

kernel-deps:
	./kernel/get-deps
	touch kernel-deps

.PHONY: kernel
kernel: kernel-deps
	$(MAKE) -C kernel DEBUG=$(DEBUG) DISABLE_FLANTERM=$(DISABLE_FLANTERM) DEBUG_LOCKS=$(DEBUG_LOCKS) CHECK_SLABS_=$(CHECK_SLABS)

$(IMAGE_NAME).iso: limine/limine kernel
	rm -rf iso_root
	mkdir -p iso_root/boot
	cp -v kernel/bin-$(KARCH)/kernel iso_root/boot/
	mkdir -p iso_root/boot/limine
	cp -v limine.conf iso_root/boot/limine/
	mkdir -p iso_root/EFI/BOOT
ifeq ($(KARCH),x86_64)
	cp -v limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin iso_root/boot/limine/
	cp -v limine/BOOTX64.EFI iso_root/EFI/BOOT/
	cp -v limine/BOOTIA32.EFI iso_root/EFI/BOOT/
	xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(IMAGE_NAME).iso
	./limine/limine bios-install $(IMAGE_NAME).iso
else ifeq ($(KARCH),aarch64)
	cp -v limine/limine-uefi-cd.bin iso_root/boot/limine
	cp -v limine/BOOTAA64.EFI iso_root/EFI/BOOT/
	xorriso -as mkisofs \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(IMAGE_NAME).iso
else ifeq ($(KARCH),riscv64)
	cp -v limine/limine-uefi-cd.bin iso_root/boot/limine
	cp -v limine/BOOTRISCV64.EFI iso_root/EFI/BOOT/
	xorriso -as mkisofs \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(IMAGE_NAME).iso
else ifeq ($(KARCH),loongarch64)
	cp -v limine/limine-uefi-cd.bin iso_root/boot/limine/
	cp -v limine/BOOTLOONGARCH64.EFI iso_root/EFI/BOOT/
	xorriso -as mkisofs \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(IMAGE_NAME).iso
endif
	rm -rf iso_root

$(IMAGE_NAME).hdd: limine kernel
	rm -f $(IMAGE_NAME).hdd
	dd if=/dev/zero bs=1M count=0 seek=64 of=$(IMAGE_NAME).hdd
	sgdisk $(IMAGE_NAME).hdd -n 1:2048 -t 1:ef00
ifeq ($(KARCH),x86_64)
	./limine/limine bios-install $(IMAGE_NAME).hdd
endif
	mformat -i $(IMAGE_NAME).hdd@@1M
	mmd -i $(IMAGE_NAME).hdd@@1M ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine
	mcopy -i $(IMAGE_NAME).hdd@@1M kernel/bin-$(KARCH)/kernel ::/boot
	mcopy -i $(IMAGE_NAME).hdd@@1M limine.conf ::/boot/limine
ifeq ($(KARCH),x86_64)
	mcopy -i $(IMAGE_NAME).hdd@@1M limine/limine-bios.sys ::/boot/limine
	mcopy -i $(IMAGE_NAME).hdd@@1M limine/BOOTX64.EFI ::/EFI/BOOT
	mcopy -i $(IMAGE_NAME).hdd@@1M limine/BOOTIA32.EFI ::/EFI/BOOT
else ifeq ($(KARCH),aarch64)
	mcopy -i $(IMAGE_NAME).hdd@@1M limine/BOOTAA64.EFI ::/EFI/BOOT
else ifeq ($(KARCH),riscv64)
	mcopy -i $(IMAGE_NAME).hdd@@1M limine/BOOTRISCV64.EFI ::/EFI/BOOT
else ifeq ($(KARCH),loongarch64)
	mcopy -i $(IMAGE_NAME).hdd@@1M limine/BOOTLOONGARCH64.EFI ::/EFI/BOOT
endif

.PHONY: clean
clean:
	if test -f kernel-deps; then $(MAKE) -C kernel clean; fi
	rm -rf iso_root-$(KARCH)/boot/limine || true
	rm iso_root-$(KARCH)/boot/kernel || true

	rm $(IMAGE_NAME).iso $(IMAGE_NAME).hdd || true

.PHONY: distclean
distclean:
	if test -f kernel-deps; then $(MAKE) -C kernel distclean; fi
	rm -rf iso_root-* *.iso *.hdd kernel-deps limine ovmf*
