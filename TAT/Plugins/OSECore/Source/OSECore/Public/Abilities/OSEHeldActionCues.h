// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "GameplayTagContainer.h"

#include "OSEHeldActionCues.generated.h"

USTRUCT(BlueprintType)
struct OSECORE_API FOSEHeldActionCues
{
   GENERATED_BODY()

public:
   // Optional cue played for the duration of held action
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "GameplayCue"))
   FGameplayTag Cue_HoldDuration;

   // Optional cue played if action is interrupted before duration elapses
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "GameplayCue"))
   FGameplayTag Cue_HoldInterrupted;

   // Optional cue played when duration elapses without held action being interrupted
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "GameplayCue"))
   FGameplayTag Cue_HoldCompleted;
};
