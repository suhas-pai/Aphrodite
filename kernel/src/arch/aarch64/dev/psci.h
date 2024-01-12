/*
 * kernel/src/arch/aarch64/dev/psci.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

enum psci_function {
    PSCI_FUNC_VERSION,
    PSCI_FUNC_CPU_OFF,
    PSCI_FUNC_MIGRATE_INFO_TYPE,
    PSCI_FUNC_SYSTEM_OFF,
    PSCI_FUNC_SYSTEM_RESET,
    PSCI_FUNC_CPU_SUSPEND,
    PSCI_FUNC_CPU_ON,
    PSCI_FUNC_AFFINITY_INFO,
    PSCI_FUNC_MIGRATE,
    PSCI_FUNC_MIGRATE_INFO_UP_CPU
};

enum psci_return_value {
    PSCI_RETVAL_SUCCESS,
    PSCI_RETVAL_NOT_SUPPORTED    = -1,
    PSCI_RETVAL_INVALID_PARAMS   = -2,
    PSCI_RETVAL_DENIED           = -3,
    PSCI_RETVAL_ALREADY_ON       = -4,
    PSCI_RETVAL_ON_PENDING       = -5,
    PSCI_RETVAL_INTERNAL_FAILURE = -6,
    PSCI_RETVAL_NOT_PRESENT      = -7,
    PSCI_RETVAL_DISABLED         = -8,
};

enum psci_invoke_method {
    PSCI_INVOKE_METHOD_NONE,
    PSCI_INVOKE_METHOD_HVC,
    PSCI_INVOKE_METHOD_SMC
};

void psci_init_from_acpi(bool use_hvc);

struct devicetree;
struct devicetree_node;

bool
psci_init_from_dtb(const struct devicetree *tree,
                   const struct devicetree_node *node);

enum psci_return_value
psci_invoke_function(enum psci_function func,
                     uint64_t arg1,
                     uint64_t arg2,
                     uint64_t arg3);

enum psci_return_value psci_reboot();
enum psci_return_value psci_shutdown();