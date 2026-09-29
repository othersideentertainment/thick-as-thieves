// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "AI/Services/BTService_RunUtilityBehaviors.h"
#include "TATBTService_RunUtilityBehaviorsWithInjectionSupport.generated.h"

class UTATUtilityAIBehaviorComponent;

UCLASS()
class TAT_API UTATBTService_RunUtilityBehaviorsWithInjectionSupport : public UBTService_RunUtilityBehaviors
{
   GENERATED_BODY()

public:
   UTATBTService_RunUtilityBehaviorsWithInjectionSupport(const FObjectInitializer& objectInitializer);
   virtual void OnInstanceCreated(UBehaviorTreeComponent& ownerComp) override;
protected:
   virtual void SetBehaviors(UUtilityAIComponent& utilityAIComponent) override;
   virtual bool ShouldSetBehaviors(const UUtilityAIComponent& utilityAIComponent) const override;

   void OnBehaviorInjectionChanges(UTATUtilityAIBehaviorComponent* behaviorComponent);
   virtual void CreateBehaviorInstances(UUtilityAIBehaviorComponent* utilityAIBehaviorComponent) override;

   UPROPERTY(EditAnywhere, Category = "Node")
   FGameplayTag BehaviorInjectionTag;
   bool IsBehaviorCreationEnabled { false };
   bool StatesHaveChanged { true };
};
