// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/Effects/TATClueEffectHelpers.h"

// tat
#include "Variation/Clues/Effects/TATClueEffect.h"

// ue
#include "StructUtils/InstancedStruct.h"


void TATClueEffectHelpers::ApplyEffects(TConstArrayView<FInstancedStruct> clueEffects, APlayerState* playerState)
{
   for(const FInstancedStruct& instance : clueEffects)
   {
      if(const FTATClueEffect* effect = instance.GetPtr<FTATClueEffect>())
      {
         effect->ApplyTo(playerState);
      }
   }
}

#if WITH_EDITOR
void TATClueEffectHelpers::ValidateEffects(TConstArrayView<FInstancedStruct> clueEffects, TFunctionRef<void(const FText&)> reportError)
{
   for(int i = 0; i < clueEffects.Num(); ++i)
   {
      const FTATClueEffect* effect = clueEffects[i].GetPtr<FTATClueEffect>();
      
      if(effect == nullptr)
      {
         reportError(FText::Format(INVTEXT("No effect at index {0}"), i));
      }
      else
      {
         effect->Validate([&reportError, i](const FText& message)
         {
            reportError(FText::Format(INVTEXT("Effect index {0}: {1}"), i, message));
         });
      }
   }
}
#endif
