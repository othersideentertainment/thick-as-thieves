// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "VoiceOver/OSEVoiceOverLineRequestParams.h"

// ue5
#include "GameplayTagContainer.h"

#include "OSEVoiceOverTriggers.generated.h"

struct FGameplayEffectSpec;
class AOSECharacterBase;

USTRUCT(BlueprintType)
struct FOSEDamageVoiceOverTrigger
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OSE|Audio", meta = (Categories = "DamageSource"))
   FGameplayTag Tag;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OSE|Audio")
   FOSEVoiceOverLineRequestParams VoiceOverParams;
};

UCLASS()
class OSECORE_API UOSEVoiceOverTriggers : public UDataAsset
{
   GENERATED_BODY()

   // Damage VO triggers
   UPROPERTY(EditDefaultsOnly, Category = "OSE|Audio")
   TArray<FOSEDamageVoiceOverTrigger> DamageTriggers;

public:
   void FireDamageVO(AActor* actor, const FGameplayEffectSpec& spec);
};
