// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "GameplayBehaviorConfig.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayBehaviorConfig_Ambient.generated.h"

USTRUCT()
struct FAmbientBehaviorDefinition
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly)
   FGameplayTagQuery QueryToMatchForValid;

   UPROPERTY(EditDefaultsOnly)
   TSubclassOf<UGameplayAbility> AbilityToActivate;   
};

UCLASS()
class TAT_API UGameplayBehaviorConfig_Ambient : public UGameplayBehaviorConfig
{
   GENERATED_BODY()

public:
   UGameplayBehaviorConfig_Ambient();
   
   UPROPERTY(EditAnywhere, Category = "SmartObject|Default")
   TSubclassOf<UGameplayAbility> AbilityToActivate;

   UPROPERTY(EditAnywhere, Category = "SmartObject|Specific Interactions")
   TArray<FAmbientBehaviorDefinition> SpecificInteractions; 
};
