// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/Utility/TATAIStateCombination.h"

// ue
#include "Engine/DataAsset.h"

#include "TATAIStatesSpeedMapping.generated.h"

class UTATCharacterAIMovement;

UCLASS(BlueprintType)
class TAT_API UTATAIStatesSpeedMapping : public UDataAsset
{
   GENERATED_BODY()

   // from UObject
#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

public:
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (UIMin = "0.0", ClampMin = "0.0"))
   TMap<FTATAIStateCombination, float> StatesToSpeeds;

   // Lookup the provided state in StatesToSpeeds and, if found, apply that speed as a base speed override to the movement component.
   UFUNCTION(BlueprintCallable)
   void ApplyMovementSpeedOverride(UTATCharacterAIMovement* aiMovementComponent, const FTATAIStateCombination& state);

   // Clears the base speed override in the movement component.
   UFUNCTION(BlueprintCallable)
   void ClearMovementSpeedOverride(UTATCharacterAIMovement* aiMovementComponent);
};

DECLARE_LOG_CATEGORY_EXTERN(LogTATAIStatesSpeedMapping, Log, All);
