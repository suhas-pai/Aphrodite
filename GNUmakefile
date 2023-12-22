# Nuke built-in rules and variables.
override MAKEFLAGS += -rR

override IMAGE_NAME := template
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

# Toolchain for building the 'limine' executable for the host.
override DEFAULT_HOST_CC := cc
$(eval $(call DEFAULT_VAR,HOST_CC,$(DEFAULT_HOST_CC)))
override DEFAULT_HOST_CFLAGS := -g -O2 -pipe
$(eval $(call DEFAULT_VAR,HOST_CFLAGS,$(DEFAULT_HOST_CFLAGS)))
override DEFAULT_HOST_CPPFLAGS :=
$(eval $(call DEFAULT_VAR,HOST_CPPFLAGS,$(DEFAULT_HOST_CPPFLAGS)))
override DEFAULT_HOST_LDFLAGS :=
$(eval $(call DEFAULT_VAR,HOST_LDFLAGS,$(DEFAULT_HOST_LDFLAGS)))
override DEFAULT_HOST_LIBS :=
$(eval $(call DEFAULT_VAR,HOST_LIBS,$(DEFAULT_HOST_LIBS)))

# Target architecture to build for. Default to x86_64.
override DEFAULT_ARCH := x86_64
$(eval $(call DEFAULT_VAR,ARCH,$(DEFAULT_ARCH)))

override DEFAULT_MEM := 4G
$(eval $(call DEFAULT_VAR,MEM,$(DEFAULT_MEM)))

override DEFAULT_SMP := 4
$(eval $(call DEFAULT_VAR,SMP,$(DEFAULT_SMP)))

EXTRA_QEMU_ARGS=-d guest_errors -d unimp -d int -D ./log.txt -rtc base=localtime
ifeq ($(DEBUG), 1)
	EXTRA_QEMU_ARGS += -s -S -serial stdio
else ifeq ($(CONSOLE), 1)
	EXTRA_QEMU_ARGS += -s -S -monitor stdio -no-reboot -no-shutdown
else
	EXTRA_QEMU_ARGS += -serial stdio
endif

DEFAULT_MACHINE=virt
ifeq ($(ARCH), x86_64)
	DEFAULT_MACHINE=q35
endif

MACHINE=$(DEFAULT_MACHINE)
ifeq ($(DISABLE_ACPI), 1)
	MACHINE=$(DEFAULT_MACHINE),acpi=off
endif

ifneq ($(DEBUG), 1)
	ifeq ($(ARCH),aarch64)
		ifeq ($(shell uname -s),Darwin)
			ifeq ($(shell uname -p),arm)
				EXTRA_QEMU_ARGS += -accel hvf
			endif
		endif
	endif
endif

DEFAULT_DRIVE_KIND=block
ifeq ($(ARCH), riscv64)
	DEFAULT_DRIVE_KIND=scsi
endif

$(eval $(call DEFAULT_VAR,DRIVE_KIND,$(DEFAULT_DRIVE_KIND)))

VIRTIO_CD_QEMU_ARG=""
VIRTIO_HDD_QEMU_ARG=""

ifeq ($(DRIVE_KIND) ,block)
	ifeq ($(ARCH), riscv64)
		$(error "block device not supported on riscv64")
	endif

	DRIVE_CD_QEMU_ARG=-cdrom $(IMAGE_NAME).iso
	DRIVE_HDD_QEMU_ARG=-hda $(IMAGE_NAME).hdd
else ifeq ($(DRIVE_KIND), scsi)
	DRIVE_CD_QEMU_ARG=-device virtio-scsi-pci,id=scsi -device scsi-cd,drive=cd0 -drive id=cd0,if=none,format=raw,file=$(IMAGE_NAME).iso
	DRIVE_HDD_QEMU_ARG=-device virtio-scsi-pci,id=scsi -device scsi-hd,drive=hd0 -drive id=hd0,format=raw,file=$(IMAGE_NAME).hdd
else
	$(error "Unrecognized value for variable DRIVE_KIND")
endif

.PHONY: all
all: $(IMAGE_NAME).iso

.PHONY: all-hdd
all-hdd: $(IMAGE_NAME).hdd

.PHONY: run
run: run-$(ARCH)

.PHONY: run-hdd
run-hdd: run-hdd-$(ARCH)

.PHONY: run-x86_64
run-x86_64: QEMU_RUN = 1
run-x86_64: ovmf $(IMAGE_NAME).iso
	qemu-system-x86_64 -M $(MACHINE) -cpu max -m $(MEM) -bios ovmf-x86_64/OVMF.fd -cdrom $(IMAGE_NAME).iso -boot d $(EXTRA_QEMU_ARGS) -smp $(SMP)

.PHONY: run-hdd-x86_64
run-hdd-x86_64: QEMU_RUN = 1
run-hdd-x86_64: ovmf $(IMAGE_NAME).hdd
	qemu-system-x86_64 -M $(MACHINE) -cpu max -m $(MEM) -bios ovmf-x86_64/OVMF.fd -hda $(IMAGE_NAME).hdd $(EXTRA_QEMU_ARGS) -smp $(SMP)

.PHONY: run-aarch64
run-aarch64: QEMU_RUN = 1
run-aarch64: ovmf $(IMAGE_NAME).iso
	qemu-system-aarch64 -M $(MACHINE) -cpu max -device ramfb -device qemu-xhci -device usb-kbd -m $(MEM) -bios ovmf-$(ARCH)/OVMF.fd $(DRIVE_CD_QEMU_ARG) -boot d $(EXTRA_QEMU_ARGS) -smp $(SMP)

.PHONY: run-hdd-aarch64
run-hdd-aarch64: QEMU_RUN = 1
run-hdd-aarch64: ovmf $(IMAGE_NAME).hdd
	qemu-system-aarch64 -M $(MACHINE) -cpu max -device ramfb -device qemu-xhci -device usb-kbd -m $(MEM) -bios ovmf-aarch64/OVMF.fd $(DRIVE_HDD_QEMU_ARG) $(EXTRA_QEMU_ARGS) -smp $(SMP)

.PHONY: run-riscv64
run-riscv64: QEMU_RUN = 1
run-riscv64: ovmf $(IMAGE_NAME).iso
	qemu-system-riscv64 -M $(MACHINE) -cpu max -device ramfb -device qemu-xhci -device usb-kbd -m $(MEM) $(DRIVE_CD_QEMU_ARG) $(EXTRA_QEMU_ARGS) -smp $(SMP)

.PHONY: run-hdd-riscv64
run-hdd-riscv64: QEMU_RUN = 1
run-hdd-riscv64: ovmf $(IMAGE_NAME).hdd
	qemu-system-riscv64 -M $(MACHINE) -cpu max -device ramfb -device qemu-xhci -device usb-kbd -m $(MEM) $(DRIVE_HDD_QEMU_ARG) $(EXTRA_QEMU_ARGS) -smp $(SMP)

.PHONY: run-bios
run-bios: QEMU_RUN = 1
run-bios: $(IMAGE_NAME).iso
	qemu-system-x86_64 -M $(MACHINE) -cpu max -m $(MEM) -cdrom $(IMAGE_NAME).iso -boot d $(EXTRA_QEMU_ARGS) -smp $(SMP)

.PHONY: run-hdd-bios
run-hdd-bios: QEMU_RUN = 1
run-hdd-bios: $(IMAGE_NAME).hdd
	qemu-system-x86_64 -M $(MACHINE) -cpu max -m $(MEM) -hda $(IMAGE_NAME).hdd $(EXTRA_QEMU_ARGS) -smp $(SMP)

.PHONY: ovmf
ovmf: ovmf-$(ARCH)

ovmf-x86_64:
	mkdir -p ovmf-x86_64
	cd ovmf-x86_64 && curl -o OVMF.fd https://retrage.github.io/edk2-nightly/bin/RELEASEX64_OVMF.fd

ovmf-aarch64:
	mkdir -p ovmf-aarch64
	cd ovmf-aarch64 && curl -o OVMF.fd https://retrage.github.io/edk2-nightly/bin/RELEASEAARCH64_QEMU_EFI.fd

ovmf-riscv64:
	mkdir -p ovmf-riscv64
	cd ovmf-riscv64 && curl -o OVMF.fd https://retrage.github.io/edk2-nightly/bin/RELEASERISCV64_VIRT_CODE.fd && dd if=/dev/zero of=OVMF.fd bs=1 count=0 seek=33554432

limine:
	git clone https://github.com/limine-bootloader/limine.git --branch=v6.x-branch-binary --depth=1
	$(MAKE) -C limine \
		CC="$(HOST_CC)" \
		CFLAGS="$(HOST_CFLAGS)" \
		CPPFLAGS="$(HOST_CPPFLAGS)" \
		LDFLAGS="$(HOST_LDFLAGS)" \
		LIBS="$(HOST_LIBS)"

.PHONY: kernel
kernel:
	$(MAKE) -C kernel DEBUG=$(DEBUG)

$(IMAGE_NAME).iso: limine kernel
	rm -rf iso_root
	mkdir -p iso_root
	cp kernel/bin/kernel limine.cfg iso_root/
	mkdir -p iso_root/EFI/BOOT
ifeq ($(ARCH),x86_64)
	cp -v limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin iso_root/
	cp -v limine/BOOTX64.EFI iso_root/EFI/BOOT/
	cp -v limine/BOOTIA32.EFI iso_root/EFI/BOOT/
	xorriso -as mkisofs -b limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(IMAGE_NAME).iso
	./limine/limine bios-install $(IMAGE_NAME).iso
else ifeq ($(ARCH),aarch64)
	cp -v limine/limine-uefi-cd.bin iso_root/
	cp -v limine/BOOTAA64.EFI iso_root/EFI/BOOT/
	xorriso -as mkisofs \
		--efi-boot limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(IMAGE_NAME).iso
else ifeq ($(ARCH),riscv64)
	cp -v limine/limine-uefi-cd.bin iso_root/
	cp -v limine/BOOTRISCV64.EFI iso_root/EFI/BOOT/
	xorriso -as mkisofs \
		--efi-boot limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(IMAGE_NAME).iso
endif
	rm -rf iso_root

$(IMAGE_NAME).hdd: limine kernel
	rm -f $(IMAGE_NAME).hdd
	dd if=/dev/zero bs=1M count=0 seek=64 of=$(IMAGE_NAME).hdd
	sgdisk $(IMAGE_NAME).hdd -n 1:2048 -t 1:ef00
ifeq ($(ARCH),x86_64)
	./limine/limine bios-install $(IMAGE_NAME).hdd
endif
	mformat -i $(IMAGE_NAME).hdd@@1M
	mmd -i $(IMAGE_NAME).hdd@@1M ::/EFI ::/EFI/BOOT
	mcopy -i $(IMAGE_NAME).hdd@@1M kernel/bin/kernel limine.cfg ::/
ifeq ($(ARCH),x86_64)
	mcopy -i $(IMAGE_NAME).hdd@@1M limine/limine-bios.sys ::/EFI/BOOT
	mcopy -i $(IMAGE_NAME).hdd@@1M limine/BOOTX64.EFI ::/EFI/BOOT
	mcopy -i $(IMAGE_NAME).hdd@@1M limine/BOOTIA32.EFI ::/EFI/BOOT
else ifeq ($(ARCH),aarch64)
	mcopy -i $(IMAGE_NAME).hdd@@1M limine/BOOTAA64.EFI ::/EFI/BOOT
else ifeq ($(ARCH),riscv64)
	mcopy -i $(IMAGE_NAME).hdd@@1M limine/BOOTRISCV64.EFI ::/EFI/BOOT
endif

.PHONY: clean
clean:
	rm -rf iso_root $(IMAGE_NAME).iso $(IMAGE_NAME).hdd
	$(MAKE) -C kernel clean

.PHONY: distclean
distclean: clean
	rm -rf limine ovmf*
	$(MAKE) -C kernel distclean
