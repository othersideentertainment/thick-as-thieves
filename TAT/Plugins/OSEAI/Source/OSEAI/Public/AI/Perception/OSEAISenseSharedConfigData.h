// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"

#include "OSEAISenseSharedConfigData.generated.h"

class UAIPerceptionComponent;

USTRUCT()
struct FOSEAISenseSharedConfig_GameplayTagPerceptionModifiers
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere)
   FGameplayTag GameplayTag;

   UPROPERTY(EditAnywhere)
   float ModifierIfGameplayTagPresent {1.f};
};

UCLASS(ClassGroup=AI)
class OSEAI_API UOSEAISenseSharedConfigData : public UDataAsset
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere)
   TArray<FOSEAISenseSharedConfig_GameplayTagPerceptionModifiers> RangePerceptionModifiers;

   float CalculateRangePerceptionModifiers(const UAIPerceptionComponent* listener) const;
};
