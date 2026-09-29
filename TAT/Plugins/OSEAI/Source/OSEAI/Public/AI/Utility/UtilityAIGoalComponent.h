// (c) 2022-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Utility/UtilityAIComponent.h"
#include "AI/Utility/UtilityAITypes.h"

// ue
#include "CoreMinimal.h"
#include "DrawDebugHelpers.h"
#include "BehaviorTree/BehaviorTreeTypes.h"

// self
#include "UtilityAIGoalComponent.generated.h"

#if ENABLE_VISUAL_LOG
struct FVisualLogEntry;
#endif // ENABLE_VISUAL_LOG

class UBlackboardData;

UCLASS(Abstract)
class OSEAI_API UUtilityAIGoalComponent : public UUtilityAIComponent
{
   GENERATED_BODY()

public:

   UUtilityAIGoalComponent();
   
   // from UActorComponent
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

public:

#if ENABLE_VISUAL_LOG
   virtual void DescribeSelfToVisLog(FVisualLogEntry* snapshot) const override;
#endif // ENABLE_VISUAL_LOG

protected:
   // from UUtilityAIComponent
   virtual void _OnStateEntered(const FUtilityStateTarget& target, const FUtilityStateEvaluatorInstance& state) override;
   virtual void _OnStateExited(const FUtilityStateTarget& target, const FUtilityStateEvaluatorInstance& state) override;
   virtual void _OnStateChanged(const FUtilityStateTarget& target, const FUtilityStateEvaluatorInstance& state) override;

#if WITH_EDITOR
   // editor utilities
   virtual void _RefreshWeights();

   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) override;
   virtual void PostLoad() override;
   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
   virtual void PostEditUndo() override;
#endif // WITH_EDITOR

protected:
   UPROPERTY(EditDefaultsOnly, Category = "AI|OSE|Utility|Blackboard Keys", DisplayName = "Goal Object")
   FName _blackboardKeyGoalObjectName;
   UPROPERTY(EditDefaultsOnly, Category = "AI|OSE|Utility|Blackboard Keys", DisplayName = "Target Actor")
   FName _blackboardKeyTargetActorName;
   UPROPERTY(EditDefaultsOnly, Category = "AI|OSE|Utility|Blackboard Keys", DisplayName = "Utility State Target")
   FName _blackboardKeyUtilityStateTarget;

   UPROPERTY(EditDefaultsOnly, Category = "AI|OSE|Utility")
   TArray<UUtilityGoalSet*> _goalSets;

private:
#if WITH_EDITORONLY_DATA
   // uneditable weights, pulls from the evaluator to display inline
   UPROPERTY(Transient, EditAnywhere, meta = (EditCondition = "false"), Category = "AI|OSE|Utility|Weights")
   TArray<FString> WeightInfo;
#endif // WITH_EDITORONLY_DATA
};
