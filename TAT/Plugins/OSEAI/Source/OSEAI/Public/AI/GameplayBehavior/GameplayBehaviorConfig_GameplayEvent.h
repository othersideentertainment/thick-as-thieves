// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "GameplayBehaviorConfig.h"
#include "GameplayTagContainer.h"

#include "GameplayBehaviorConfig_GameplayEvent.generated.h"

class UBehaviorTree;

UCLASS()
class OSEAI_API UGameplayBehaviorConfig_GameplayEvent : public UGameplayBehaviorConfig
{
   GENERATED_BODY()

public:
   
   UGameplayBehaviorConfig_GameplayEvent(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   const FGameplayTag& GetGameplayEventTag() const { return _gameplayEventTag; }

protected:
   UPROPERTY(EditAnywhere, Category = SmartObject)
   FGameplayTag _gameplayEventTag;
};
