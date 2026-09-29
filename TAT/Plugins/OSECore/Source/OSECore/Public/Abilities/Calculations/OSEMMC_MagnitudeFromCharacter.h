// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"

#include "OSEMMC_MagnitudeFromCharacter.generated.h"

class AOSECharacterBase;

// Calculates magnitude using the effect causing character.
UCLASS()
class OSECORE_API UOSEMMC_MagnitudeFromCharacter : public UGameplayModMagnitudeCalculation
{
   GENERATED_BODY()

public:
   virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& spec) const override;

   UFUNCTION(BlueprintImplementableEvent)
   float GetMagnitudeFromCharacter(AOSECharacterBase* character) const;
};
