// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

class UCanvas;
class AHUD;
class FDebugDisplayInfo;

namespace BreakableDebugVis
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
   void OnShowDebugInfo(AHUD* hud, UCanvas* canvas, const FDebugDisplayInfo& displayInfo, float& yl, float& yPos);
#endif
}
