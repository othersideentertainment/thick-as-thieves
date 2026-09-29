// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "GameplayBehaviorConfig.h"
#include "GameplayTagContainer.h"

#include "GameplayBehaviorConfig_BehaviorTreeInjection.generated.h"

class UBehaviorTree;

UCLASS()
class OSEAI_API UGameplayBehaviorConfig_BehaviorTreeInjection : public UGameplayBehaviorConfig
{
   GENERATED_BODY()

public:
   
   UGameplayBehaviorConfig_BehaviorTreeInjection(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   UBehaviorTree* GetBehaviorTree() const;
   const FGameplayTag& GetInjectionTag() const { return _injectionTag; }

protected:
   UPROPERTY(EditAnywhere, Category = SmartObject)
   mutable TSoftObjectPtr<UBehaviorTree> _behaviorTree;

   UPROPERTY(EditAnywhere, Category = SmartObject, meta = (Categories = "AI.BehaviorTree.InjectionTags"))
   FGameplayTag _injectionTag;
};
