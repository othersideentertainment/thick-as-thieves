// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ose
#include "Conditions/OSECondition.h"

// ue4
#include "CoreMinimal.h"

#include "OSEVoiceOverBucket.generated.h"

class UOSEVoiceOverBucketItem;

USTRUCT()
struct FOSEVoiceOverBucketEntry
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere)
   FOSEConditionSet Conditions;

   UPROPERTY(EditAnywhere)
   UOSEVoiceOverBucketItem* VoiceItem = nullptr;
};


UCLASS(BlueprintType)
class OSECORE_API UOSEVoiceOverBucket: public UObject
{
   GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere)
    TArray<FOSEVoiceOverBucketEntry> Entries;

    UFUNCTION()
    UAkAudioEvent* GetMostAudibleEvent(const AActor* speaker) const;
};
