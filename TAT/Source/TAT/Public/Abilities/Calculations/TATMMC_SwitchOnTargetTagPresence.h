// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"

#include "TATMMC_SwitchOnTargetTagPresence.generated.h"

/// Used to calculate different magnitudes (e.g. for duration) based on the presence of a tag
/// Currently is just a binary w/ two values if the tag is on the target or not
UCLASS()
class TAT_API UTATMMC_SwitchOnTargetTagPresence : public UGameplayModMagnitudeCalculation
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly)
   FGameplayTag TagToCheck;

   UPROPERTY(EditDefaultsOnly)
   float ValueWithTag = 1.0f;

   UPROPERTY(EditDefaultsOnly)
   float ValueWithoutTag = 1.0f;

   virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& spec) const override;
};
