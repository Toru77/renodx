#pragma once

#include <cstddef>
#include <cstdint>

#include <Rtxdi/DI/ReSTIRDI.h>

static_assert(sizeof(RTXDI_PackedDIReservoir) == 24);
static_assert(offsetof(RTXDI_PackedDIReservoir, lightData) == 0);
static_assert(offsetof(RTXDI_PackedDIReservoir, uvData) == 4);
static_assert(offsetof(RTXDI_PackedDIReservoir, mVisibility) == 8);
static_assert(offsetof(RTXDI_PackedDIReservoir, distanceAge) == 12);
static_assert(offsetof(RTXDI_PackedDIReservoir, targetPdf) == 16);
static_assert(offsetof(RTXDI_PackedDIReservoir, weight) == 20);
static_assert(RTXDI_RESERVOIR_BLOCK_SIZE == 16);
