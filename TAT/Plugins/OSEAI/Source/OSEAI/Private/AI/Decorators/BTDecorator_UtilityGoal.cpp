// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Decorators/BTDecorator_UtilityGoal.h"

// ose
#include "AI/OSEAIController.h"
#include "AI/Utility/UtilityAIGoalComponent.h"

// ue4
#include "Engine/World.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/Composites/BTComposite_SimpleParallel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTDecorator_UtilityGoal)

UBTDecorator_UtilityGoal::UBTDecorator_UtilityGoal(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   NodeName = "Utility Goal";
   bCreateNodeInstance = true;
}

void UBTDecorator_UtilityGoal::OnInstanceCreated(UBehaviorTreeComponent& ownerComp)
{
   Super::OnInstanceCreated(ownerComp);

   _goalComponent = _GetUtilityAIGoalComponent(ownerComp);

   if (IsValid(_goalComponent))
   {
      _goalComponent->OnAIStateExit.AddUniqueDynamic(this, &UBTDecorator_UtilityGoal::_OnGoalExited);
   }
}

void UBTDecorator_UtilityGoal::OnInstanceDestroyed(UBehaviorTreeComponent& ownerComp)
{
   Super::OnInstanceDestroyed(ownerComp);

   if (IsValid(_goalComponent))
   {
      _goalComponent->OnAIStateEnter.RemoveAll(this);
   }
}

bool UBTDecorator_UtilityGoal::CalculateRawConditionValue(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory) const
{
   bool superCanExecute = Super::CalculateRawConditionValue(ownerComp, nodeMemory);
   bool canExecute = false;
   if (IsValid(_goalComponent))
   {
      const FUtilityStateEvaluatorInstance& currentState = _goalComponent->GetCurrentEvaluatorInstance();
      if (currentState.IsValid() && currentState.Instance->GetClass()->IsChildOf(GoalClass))
      {
         canExecute = true;
      }
   }
   return superCanExecute && canExecute;
}

void UBTDecorator_UtilityGoal::_OnGoalExited(const FUtilityStateEvaluatorInstance& goalState, const FUtilityStateTarget& target)
{
   _TryConditionalFlowAbort(goalState);
}

FString UBTDecorator_UtilityGoal::GetStaticDescription() const
{
   return FString::Printf(TEXT("Execute when goal is %s"), GoalClass ? *GoalClass->GetName() : TEXT("None"));
}

void UBTDecorator_UtilityGoal::_TryConditionalFlowAbort(const FUtilityStateEvaluatorInstance& goalState)
{
   if (IsValid(_goalComponent))
   {
      if (goalState.IsValid() && goalState.Instance->GetClass()->IsChildOf(GoalClass))
      {
         if (AOSEAIController* controller = Cast<AOSEAIController>(_goalComponent->GetOwner()))
         {
            if (UBehaviorTreeComponent* treeComp = Cast<UBehaviorTreeComponent>(controller->BrainComponent))
            {
               if (treeComp->IsExecutingBranch(this, GetChildIndex()))
               {
                  ConditionalFlowAbort(*treeComp, EBTDecoratorAbortRequest::ConditionPassing);
               }
            }
         }
      }
   }
}

UUtilityAIGoalComponent* UBTDecorator_UtilityGoal::_GetUtilityAIGoalComponent(UBehaviorTreeComponent& ownerComp) const
{
   if (AOSEAIController* controller = Cast<AOSEAIController>(ownerComp.GetOwner()))
   {
      return controller->GetUtilityAIGoalComponent();
   }
   return nullptr;
}


#if WITH_EDITOR
FName UBTDecorator_UtilityGoal::GetNodeIconName() const
{
   return FName("BTEditor.Graph.BTNode.Decorator.Conditional.Icon");
}
#endif // WITH_EDITOR

