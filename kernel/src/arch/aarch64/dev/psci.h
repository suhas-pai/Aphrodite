/*
 * kernel/src/arch/aarch64/dev/psci.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

enum psci_function {
    PSCI_FUNC_VERSION             = 0x84000000,
    PSCI_FUNC_CPU_OFF             = 0x84000002,
    PSCI_FUNC_MIGRATE_INFO_TYPE   = 0x84000006,
    PSCI_FUNC_SYSTEM_OFF          = 0x84000008,
    PSCI_FUNC_SYSTEM_RESET        = 0x84000009,
    PSCI_FUNC_CPU_SUSPEND         = 0xC4000001,
    PSCI_FUNC_CPU_ON              = 0xC4000003,
    PSCI_FUNC_AFFINITY_INFO       = 0xC4000004,
    PSCI_FUNC_MIGRATE             = 0xC4000005,
    PSCI_FUNC_MIGRATE_INFO_UP_CPU = 0xC4000007,
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

enum psci_return_value
psci_invoke_function(enum psci_function func,
                     uint64_t arg1,
                     uint64_t arg2,
                     uint64_t arg3);

enum psci_return_value psci_reboot();
enum psci_return_value psci_shutdown();