/*
 * kernel/include/dev/init.h
 * © suhas pai
 */

#pragma once

void dev_init();
void serial_init();

struct bus *dev_root_bus();
void dev_init_drivers();

typedef void (*dev_init_t)();

extern char dev_inits_start[];
extern char dev_inits_end[];

#define dev_inits_foreach(iter) \
    ptrrange_foreach(cast_to_ptr(dev_init_t, dev_inits_start), \
                     cast_to_ptr(dev_init_t, dev_inits_end), \
                     iter)

#define __dev_init __attribute__((used, section(".dev_inits")))
#define MAKE_DEV_INIT_FUNC(name) \
    __dev_init static const dev_init_t h_var(init) = (name);
