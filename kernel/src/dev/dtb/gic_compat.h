/*
 * kernel/src/dev/dtb/gic_compat.h
 * © suhas pai
 */

#pragma once
#include "lib/adt/string_view.h"

static const struct string_view gic_compat_sv_list[] = {
    SV_STATIC("arm,cortex-a15-gic"),
    SV_STATIC("arm,arm11mp-gic"),
    SV_STATIC("arm,cortex-a7-gic"),
    SV_STATIC("arm,cortex-a5-gic"),
    SV_STATIC("arm,cortex-a9-gic"),
    SV_STATIC("arm,eb11mp-gic"),
    SV_STATIC("arm,gic-400"),
    SV_STATIC("arm,pl390"),
    SV_STATIC("arm,tc11mp-gic"),
    SV_STATIC("nvidia,tegra210-agic"),
    SV_STATIC("qcom,msm-8660-qgic"),
    SV_STATIC("qcom,msm-qgic2")
};