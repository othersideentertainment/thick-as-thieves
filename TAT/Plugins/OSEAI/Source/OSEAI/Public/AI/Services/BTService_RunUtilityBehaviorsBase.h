// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "AI/Utility/UtilityAITypes.h"

// ue4
#include "BehaviorTree/BTService.h"

#include "BTService_RunUtilityBehaviorsBase.generated.h"

struct FBlackboardKeySelector;
class UUtilityAIBehaviorComponent;
class UUtilityAIGoalComponent;

UCLASS(Abstract)
class OSEAI_API UBTService_RunUtilityBehaviorsBase : public UBTService
{
   GENERATED_BODY()

public:
   UBTService_RunUtilityBehaviorsBase(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   // from UBTService
   virtual void InitializeFromAsset(UBehaviorTree& asset) override;
   virtual void OnInstanceCreated(UBehaviorTreeComponent& ownerComp) override;
   virtual void OnSearchStart(FBehaviorTreeSearchData& searchData) override;
   virtual void OnBecomeRelevant(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory) override;
   virtual void TickNode(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds) override;
   virtual void OnCeaseRelevant(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory) override;
   virtual void OnInstanceDestroyed(UBehaviorTreeComponent& ownerComp) override;

protected:
   virtual void SetBehaviors(UUtilityAIComponent& utilityAIComponent);
   virtual bool ShouldSetBehaviors(const UUtilityAIComponent& utilityAIComponent) const;
   UUtilityAIBehaviorComponent* _GetUtilityAIBehaviorComponent(const UBehaviorTreeComponent& ownerComp) const;
   UUtilityAIGoalComponent* _GetUtilityAIGoalComponent(const UBehaviorTreeComponent& ownerComp) const;
   
   virtual const TArray<FUtilityStateEvaluatorInstance>& GetBehaviorInstances() const { return _behaviors; };

   UPROPERTY(EditAnywhere, Category = "Blackboard")
   FBlackboardKeySelector _behaviorNameBlackboardKey;

   UPROPERTY(Transient)
   TArray<FUtilityStateEvaluatorInstance> _behaviors;

private:
   void _UpdateBlackboard(UUtilityAIComponent& utilityAIComp);
};
