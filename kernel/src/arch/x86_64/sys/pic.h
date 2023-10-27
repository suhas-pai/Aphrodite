/*
 * kernel/arch/x86_64/sys/pic.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

void pic_remap(const uint8_t offset, const uint8_t offset2);
void pic_send_eoi(const uint8_t irq);

void pic_mask_remapped_irqs();
void pic_unmask_remapped_irqs();

void pic_disable();