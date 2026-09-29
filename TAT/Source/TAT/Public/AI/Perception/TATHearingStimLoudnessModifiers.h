// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TATHearingStimLoudnessModifiers.generated.h"

USTRUCT()
struct FTATHearingStimLoudnessModifier
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly)
   float Multiplier { 0.f };
};

UCLASS(BlueprintType)
class TAT_API UTATHearingStimLoudnessModifiers : public UDataAsset
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable)
   float GetLoudnessMultiplierForMaterial(UPhysicalMaterial* material) const;
   
protected:
   UPROPERTY(EditDefaultsOnly)
   TMap<TSoftObjectPtr<UPhysicalMaterial>, FTATHearingStimLoudnessModifier> _PhysicalMaterialToLoudnessModifier;
};
