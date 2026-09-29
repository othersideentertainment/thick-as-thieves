// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"

#include "TATPeriodicChirpEmitterToolSettings.generated.h"


UCLASS(BlueprintType)
class TAT_API UTATPeriodicChirpEmitterToolSettings : public UDataAsset
{
   GENERATED_BODY()

public:
   /// How long b/w each chirp
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (ClampMin = "0", UIMin = "0"))
   float ChirpFrequency = 2.0f;

   /// How long should the chirps go on for
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (ClampMin = "0", UIMin = "0"))
   float TotalDuration = 40.0f;

   /// If this tag is found on the instigator, the first chirp will be delayed
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FGameplayTag ChirpDelayTag;
   
   /// How much to delay the initial chirp for each count of the ChirpDelayTag found on the instigator
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (ClampMin = "0", UIMin = "0"))
   float StartDelayPerTagCount = 0.0f;
};
