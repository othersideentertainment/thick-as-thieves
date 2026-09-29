// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#if TAT_VERSION_EDITION == 2
#include "TATVersionV2.h"
using UTATVersion = UTATVersionV2;
#else
#include "Common/TATVersion.h"
using UTATVersion = UTATVersionV1;
#endif

