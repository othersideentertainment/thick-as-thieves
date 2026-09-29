// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Utility/TATAIStatesSpeedMapping.h"

// tat
#include "Character/TATCharacterAIMovement.h"

// ue
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAIStatesSpeedMapping)

DEFINE_LOG_CATEGORY(LogTATAIStatesSpeedMapping);

#if WITH_EDITOR
EDataValidationResult UTATAIStatesSpeedMapping::IsDataValid(FDataValidationContext& context) const
{
   const EDataValidationResult result = Super::IsDataValid(context);

   TSet<FTATAIStateCombination> keys;

   // shouldn't have any entries that have both Alertness and Escalation disabled
   for (const auto& it : StatesToSpeeds)
   {
      if (!it.Key.IsValid())
      {
         context.AddError(FText::FromString(TEXT("Empty entry found in AIStateComboSpeeds!")));
      }
      if (it.Value < 0.0f)
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("Negative speed entry found in AIStateComboSpeeds for %s!")
            , *it.Key.ToString())));
      }

      // An edge case can occur where, because the key type is a custom data type, keys can be created
      // distinct but made equivalent afterwards. Let's check for duplicates ourselves to prevent that.
      if (keys.Contains(it.Key))
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("Found a duplicate entry for: %s!")
            , *it.Key.ToString())));
      }
      else
      {
         keys.Add(it.Key);
      }
   }

   return context.GetIssues().Num() > 0 ? EDataValidationResult::Invalid : result;
}
#endif // WITH_EDITOR

void UTATAIStatesSpeedMapping::ApplyMovementSpeedOverride(UTATCharacterAIMovement* aiMovementComponent, const FTATAIStateCombination& state)
{
   if (!IsValid(aiMovementComponent))
   {
      UE_LOG(LogTATAIStatesSpeedMapping, Error, TEXT("UTATAIStatesSpeedMapping::ApplyMovementSpeedOverride called with an invalid UTATCharacterAIMovement!"));
      return;
   }

   if (!state.IsValid())
   {
      UE_LOG(LogTATAIStatesSpeedMapping, Error, TEXT("UTATAIStatesSpeedMapping::ApplyMovementSpeedOverride called with an invalid FTATAIStateCombination!"));
      return;
   }

   if (float* speed = StatesToSpeeds.Find(state))
   {
      aiMovementComponent->SetOverrideBaseMaxWalkSpeed(*speed);
   }
   else
   {
      aiMovementComponent->ClearOverrideBaseMaxWalkSpeed();
   }
}

void UTATAIStatesSpeedMapping::ClearMovementSpeedOverride(UTATCharacterAIMovement* aiMovementComponent)
{
   if (!IsValid(aiMovementComponent))
   {
      UE_LOG(LogTATAIStatesSpeedMapping, Error, TEXT("UTATAIStatesSpeedMapping::ClearMovementSpeedOverride called with an invalid UTATCharacterAIMovement!"));
      return;
   }

   aiMovementComponent->ClearOverrideBaseMaxWalkSpeed();
}
