// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Services/BTService_RunGoalTargetedUtilityBehaviors.h"

// ose
#include "AI/Utility/UtilityAIBehaviorComponent.h"
#include "AI/Utility/UtilityAIGoalComponent.h"

// ue4
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTService_RunGoalTargetedUtilityBehaviors)

UBTService_RunGoalTargetedUtilityBehaviors::UBTService_RunGoalTargetedUtilityBehaviors(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   NodeName = "Run Goal-Targeted Utility Behaviors";
}

void UBTService_RunGoalTargetedUtilityBehaviors::OnSearchStart(FBehaviorTreeSearchData& searchData)
{
   // tick is called immediately after this so we dont have to call _UpdateTarget() here
   Super::OnSearchStart(searchData);
}

void UBTService_RunGoalTargetedUtilityBehaviors::TickNode(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds)
{
   _UpdateTarget(ownerComp);
   Super::TickNode(ownerComp, nodeMemory, deltaSeconds);
}

void UBTService_RunGoalTargetedUtilityBehaviors::OnCeaseRelevant(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory)
{
   if (UUtilityAIBehaviorComponent* utilityAIComponent = _GetUtilityAIBehaviorComponent(ownerComp))
   {
      if (utilityAIComponent->GetForcedTargetOwner() == this)
      {
         _ClearTargets(ownerComp);
      }
   }
   Super::OnCeaseRelevant(ownerComp, nodeMemory);
}

FString UBTService_RunGoalTargetedUtilityBehaviors::GetStaticDescription() const
{
#if WITH_EDITORONLY_DATA
   return FString::Printf(TEXT("%d behavior %s containing %d behaviors (targeted)")
      , _behaviorSets.Num()
      , _behaviorSets.Num() == 1 ? TEXT("set") : TEXT("sets")
      , NumBehaviors);
#else
   return Super::GetStaticDescription();
#endif // WITH_EDITORONLY_DATA
}

void UBTService_RunGoalTargetedUtilityBehaviors::_UpdateTarget(UBehaviorTreeComponent& ownerComp)
{
   UUtilityAIGoalComponent* utilityAIGoalComponent = _GetUtilityAIGoalComponent(ownerComp);
   if (utilityAIGoalComponent)
   {
      // clear
      _ClearTargets(ownerComp);

      // add
      const FUtilityStateEvaluatorInstance& goalState = utilityAIGoalComponent->GetCurrentEvaluatorInstance();
      const FUtilityStateTarget& goalTarget = utilityAIGoalComponent->GetCurrentTarget();
      if (goalState.IsValid())
      {
         UtilityTargetingUtl::ForEachTargetingType(goalState.Evaluator->TargetingFlags, goalState.Evaluator->TargetingGroup,
            [this, &ownerComp, &goalTarget](EUtilityStateTargeting targetingType, const FGameplayTag& targetingGroup)
         {
            _AddTarget(ownerComp, targetingType, goalTarget);
         });
      }
      else
      {
         _ClearTargets(ownerComp);
      }
   }
}

void UBTService_RunGoalTargetedUtilityBehaviors::_AddTarget(UBehaviorTreeComponent& ownerComp, EUtilityStateTargeting type, const FUtilityStateTarget& target)
{
   if (UUtilityAIBehaviorComponent* utilityAIBehaviorComponent = _GetUtilityAIBehaviorComponent(ownerComp))
   {
      utilityAIBehaviorComponent->AddForcedTargetForType(this, type, target);
   }
}

void UBTService_RunGoalTargetedUtilityBehaviors::_ClearTargets(UBehaviorTreeComponent& ownerComp)
{
   if (UUtilityAIBehaviorComponent* utilityAIBehaviorComponent = _GetUtilityAIBehaviorComponent(ownerComp))
   {
      utilityAIBehaviorComponent->ClearForcedTargetForType();
   }
}

