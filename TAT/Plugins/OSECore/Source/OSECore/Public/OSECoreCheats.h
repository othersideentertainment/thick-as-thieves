// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Misc/Build.h"


// @TODO: AUDIT SHIPPING
// This is to force cheats in all (shipping) builds, typically for video recording purposes.
// Enable this only when absolutely necessary, and disable this when no longer needed.
#if !defined(OSE_CHEATS_FORCE_ON)
#   define OSE_CHEATS_FORCE_ON (0)
#endif

/// Whether or not cheats are enabled. This uses the same conditions UE does, with
/// an additional check for whether cheats are forced on or not
#define OSE_CHEATS_ENABLED (OSE_CHEATS_FORCE_ON || !(UE_BUILD_SHIPPING || UE_BUILD_TEST))
