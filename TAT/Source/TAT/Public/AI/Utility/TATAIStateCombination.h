// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Alertness/AlertnessEnums.h"

// tat
#include "AI/Escalation/TATEscalationState.h"

#include "TATAIStateCombination.generated.h"

USTRUCT(BlueprintType)
struct TAT_API FTATAIStateCombination
{
   GENERATED_BODY()

   // If false, Alertness is not specified - Alertness will not be factored into any comparisons with other state combinations.
   UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
   bool UseAlertnessFilter = false;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "UseAlertnessFilter"))
   EAlertnessLevel Alertness = EAlertnessLevel::Neutral;

   // If false, Escalation is not specified - Escalation will not be factored into any comparisons with other state combinations.
   UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
   bool UseEscalationFilter = false;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "UseEscalationFilter"))
   ETATEscalationState Escalation = ETATEscalationState::Fresh;

   FORCEINLINE bool IsValid() const { return UseAlertnessFilter || UseEscalationFilter; }

   FString ToString() const
   {
      if (UseAlertnessFilter && UseEscalationFilter)
      {
         return FString::Printf(TEXT("%s|%s"), 
            *UEnum::GetValueAsString(Alertness), 
            *UEnum::GetValueAsString(Escalation));
      }
      else if (UseAlertnessFilter)
      {
         return UEnum::GetValueAsString(Alertness);
      }
      else if (UseEscalationFilter)
      {
         return UEnum::GetValueAsString(Escalation);
      }
      else
      {
         return TEXT("Empty");
      }
   }

   FORCEINLINE bool operator==(const FTATAIStateCombination& other) const
   {
      return (!UseAlertnessFilter || !other.UseAlertnessFilter || Alertness == other.Alertness)
         && (!UseEscalationFilter || !other.UseEscalationFilter || Escalation == other.Escalation);
   }

   FORCEINLINE friend uint32 GetTypeHash(const FTATAIStateCombination& state)
   {
      if (state.UseAlertnessFilter && state.UseEscalationFilter)
      {
         return HashCombine(GetTypeHash(state.Alertness), GetTypeHash(state.Escalation));
      }
      else if (state.UseAlertnessFilter)
      {
         return GetTypeHash(state.Alertness);
      }
      else if (state.UseEscalationFilter)
      {
         return GetTypeHash(state.Escalation);
      }
      else
      {
         return 0;
      }
   }
};
