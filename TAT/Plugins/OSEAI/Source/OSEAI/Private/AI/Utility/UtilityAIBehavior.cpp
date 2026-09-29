// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Utility/UtilityAIBehavior.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"
#include "AI/OSEAIController.h"
#include "AI/OSEAISettings.h"
#include "AI/Utility/UtilityAIComponent.h"

// ue4
#include "BrainComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UtilityAIBehavior)

///////////////////////////////////////////////////////////////////
///        UUtilityAIBehavior
///////////////////////////////////////////////////////////////////

UUtilityAIBehavior::UUtilityAIBehavior()
   : Super()
{
}

void UUtilityAIBehavior::Init(UUtilityAIComponent& utilityAIComponent)
{
   _ResetRuntimeState();
   Super::Init(utilityAIComponent);
}

void UUtilityAIBehavior::Reset()
{
   _ResetRuntimeState();
   Super::Reset();
}

void UUtilityAIBehavior::Enter()
{
   if (AOSEAIController* aiController = GetController())
   {
      if (UBehaviorTreeComponent* behaviorTreeComponent = Cast<UBehaviorTreeComponent>(aiController->BrainComponent))
      {
         const UOSEAISettings& settings = UOSEAISettings::Get();
         UBehaviorTree* defaultBehaviorTree = settings.DefaultInjectedUtilityBehaviorTree.LoadSynchronous();
         ensure(defaultBehaviorTree);
         UBehaviorTree* treeToRun = BehaviorTree ? BehaviorTree : defaultBehaviorTree;
         behaviorTreeComponent->SetDynamicSubtree(settings.UtilityBehaviorTreeInjectionTag, treeToRun);
      }
   }

   Super::Enter();
}

void UUtilityAIBehavior::Exit()
{
   AOSEAIController* aiController = GetController();

   // skip doing work on exit if our controller is being cleaned up
   if (IsValid(aiController))
   {
      if (UBehaviorTreeComponent* behaviorTreeComponent = Cast<UBehaviorTreeComponent>(aiController->BrainComponent))
      {
         // TODO: Test just leaving this alone and letting the next Enter() cover us instead of falling
         // back.  Might reduce some sub-tree-thrashing as we swap back and forth to the same thing in some cases.
         const UOSEAISettings& settings = UOSEAISettings::Get();
         UBehaviorTree* defaultBehaviorTree = settings.DefaultInjectedUtilityBehaviorTree.LoadSynchronous();
         ensure(defaultBehaviorTree);
         behaviorTreeComponent->SetDynamicSubtree(settings.UtilityBehaviorTreeInjectionTag, defaultBehaviorTree);
      }

      BP_Exit();
   }

   Super::Exit();
}

void UUtilityAIBehavior::_ResetRuntimeState()
{
   _behaviorState = DefaultRuntimeState.IsInterruptable ? EBehaviorState::Interruptible : EBehaviorState::NotInterruptible;
}

void UUtilityAIBehavior::Complete()
{
   AOSEAIController* aiController = GetController();

   if (_utilityAIComponent && _utilityAIComponent->GetCurrentState() == this)
   {
      _behaviorState = EBehaviorState::Completed;
   }
   else if (IsValid(aiController)) // only complain if we're not doing cleanup-on-exit
   {
      UE_LOG(LogOSEUtilityAI, Warning, TEXT("Attempted to complete behavior %s when not the current active behavior."), *GetName());
   }
}

