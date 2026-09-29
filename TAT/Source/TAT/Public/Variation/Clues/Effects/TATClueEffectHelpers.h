// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

class APlayerState;
struct FInstancedStruct;

namespace TATClueEffectHelpers
{
   void ApplyEffects(TConstArrayView<FInstancedStruct> clueEffects, APlayerState* playerState);
#if WITH_EDITOR
   void ValidateEffects(TConstArrayView<FInstancedStruct> clueEffects, TFunctionRef<void (const FText&)> reportError);
#endif
};
