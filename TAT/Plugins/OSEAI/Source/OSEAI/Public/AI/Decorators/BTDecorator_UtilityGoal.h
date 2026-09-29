// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Utility/UtilityAITypes.h"

// ue4
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BehaviorTree/BTDecorator.h"

#include "BTDecorator_UtilityGoal.generated.h"

class UUtilityAIGoalComponent;
class UUtilityAIGoal;

UCLASS(HideCategories=(Condition))
class OSEAI_API UBTDecorator_UtilityGoal : public UBTDecorator
{
   GENERATED_UCLASS_BODY()

public:
   UPROPERTY(EditAnywhere, Category = "Goals")
   TSubclassOf<UUtilityAIGoal> GoalClass;

   virtual FString GetStaticDescription() const override;

protected:
   virtual void OnInstanceCreated(UBehaviorTreeComponent& ownerComp) override;
   virtual void OnInstanceDestroyed(UBehaviorTreeComponent& ownerComp) override;
   virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory) const override;

#if WITH_EDITOR
   virtual FName GetNodeIconName() const override;
#endif // WITH_EDITOR

private:
   UFUNCTION()
   void _OnGoalExited(const FUtilityStateEvaluatorInstance& goalState, const FUtilityStateTarget& target);

private:
   void _TryConditionalFlowAbort(const FUtilityStateEvaluatorInstance& goalState);
   UUtilityAIGoalComponent* _GetUtilityAIGoalComponent(UBehaviorTreeComponent& ownerComp) const;

private:
   UPROPERTY(Transient)
   UUtilityAIGoalComponent* _goalComponent = nullptr;
};
