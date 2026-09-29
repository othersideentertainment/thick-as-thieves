// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose

// ue
#include "GameplayBehaviorConfig.h"
#include "GameplayTagContainer.h"

#include "GameplayBehaviorConfig_ResolveIncorrectObjectStateGameplayEvent.generated.h"

class UBehaviorTree;

UCLASS()
class TAT_API UGameplayBehaviorConfig_ResolveIncorrectObjectStateGameplayEvent : public UGameplayBehaviorConfig
{
   GENERATED_BODY()

public:
   
   UGameplayBehaviorConfig_ResolveIncorrectObjectStateGameplayEvent(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   const FGameplayTag& GetCorrectStateGameplayEventTag() const { return _incorrectStateGameplayEventTag; }
   const FGameplayTag& GetIncorrectGameplayEventTag() const { return _correctStateGameplayEventTag; }

protected:
   UPROPERTY(EditAnywhere, Category = SmartObject)
   FGameplayTag _correctStateGameplayEventTag;
   UPROPERTY(EditAnywhere, Category = SmartObject)
   FGameplayTag _incorrectStateGameplayEventTag;
};
