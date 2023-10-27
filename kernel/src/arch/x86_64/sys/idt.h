/*
 * kernel/arch/x86_64/sys/idt.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

typedef uint8_t idt_vector_t;

#define IST_NONE 0
#define IDT_DEFAULT_FLAGS 0x8e

void idt_init();
void idt_load();
void idt_set_vector(idt_vector_t vector, uint8_t ist, uint8_t flags);

void idt_register_exception_handlers();