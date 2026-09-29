// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/StateTrees/Tasks/TATStateTreeTaskApplyGameplayEffect.h"

// ue
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AIController.h"
#include "StateTreeExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskApplyGameplayEffect)

namespace TATStateTreeTaskHelpers
{
   static UAbilitySystemComponent* RetrieveASC(AAIController* aiController, const UObject* contextOwner, const FString& contextString)
   {
      if (aiController == nullptr)
      {
         UE_VLOG(contextOwner, LogStateTree, Error, TEXT("[%s] Failed as AIController is null!"));
         return nullptr;
      }

      const IAbilitySystemInterface* abilitySystemInterface = Cast<IAbilitySystemInterface>(aiController->GetPawn());
      if (abilitySystemInterface == nullptr)
      {
         UE_VLOG(contextOwner, LogStateTree, Error, TEXT("[%s] Failed to retrieve ability system interface!"));
         return nullptr;
      }

      UAbilitySystemComponent* abilitySystemComponent = abilitySystemInterface->GetAbilitySystemComponent();
      if (abilitySystemComponent == nullptr)
      {
         UE_VLOG(contextOwner, LogStateTree, Error, TEXT("[%s] Failed to retrieve ability system component!"));
         return nullptr;
      }

      return abilitySystemComponent;
   }
}

EStateTreeRunStatus FTATStateTreeTaskApplyGameplayEffect::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   static const FString contextString = TEXT("FTATStateTreeTaskApplyGameplayEffect::EnterState");

   FInstanceDataType& instanceData = context.GetInstanceData(*this);

   if (instanceData.GameplayEffectClass == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("[%s] failed because no gameplay effect was provided!"), 
         *contextString);
      return EStateTreeRunStatus::Failed;
   }

   UAbilitySystemComponent* abilitySystemComponent = TATStateTreeTaskHelpers::RetrieveASC(instanceData.AIController,
      context.GetOwner(), contextString);
   if (abilitySystemComponent == nullptr)
   {
      // Logging handled by helper function.
      return EStateTreeRunStatus::Failed;
   }

   instanceData.AppliedEffectHandle = abilitySystemComponent->ApplyGameplayEffectToSelf(
         instanceData.GameplayEffectClass.GetDefaultObject(), 
         instanceData.Level,
         abilitySystemComponent->MakeEffectContext()
   );

   if (instanceData.AppliedEffectHandle.IsValid())
   {
      instanceData.BoundDelegateHandle = abilitySystemComponent->OnAnyGameplayEffectRemovedDelegate().AddLambda(
         [dataRef = context.GetInstanceDataStructRef(*this), handle = instanceData.AppliedEffectHandle](const FActiveGameplayEffect& effectEnded) mutable
         {
            // Should be valid so long as this binding is active.
            check(dataRef.IsValid());

            if (handle != effectEnded.Handle)
            {
               return;
            }

            if (FInstanceDataType* instanceData = dataRef.GetPtr())
            {
               instanceData->IsFinished = true;
            }
         }
      );

      return EStateTreeRunStatus::Running;
   }

   return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FTATStateTreeTaskApplyGameplayEffect::Tick(FStateTreeExecutionContext& context, const float deltaTime) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   return instanceData.IsFinished ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

void FTATStateTreeTaskApplyGameplayEffect::ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   static const FString contextString = TEXT("FTATStateTreeTaskApplyGameplayEffect::ExitState");

   FInstanceDataType& instanceData = context.GetInstanceData(*this);

   UAbilitySystemComponent* abilitySystemComponent = TATStateTreeTaskHelpers::RetrieveASC(instanceData.AIController,
      context.GetOwner(), contextString);
   if (abilitySystemComponent == nullptr)
   {
      // Logging handled by helper function.
      return;
   }

   if (instanceData.BoundDelegateHandle.IsValid())
   {
      abilitySystemComponent->OnAnyGameplayEffectRemovedDelegate().Remove(instanceData.BoundDelegateHandle);
   }

   if (!instanceData.AppliedEffectHandle.IsValid())
   {
      // May have not been applied in the first place.
      return;
   }

   if (instanceData.IsFinished)
   {
      // The effect has already been removed.
      return;
   }

   abilitySystemComponent->RemoveActiveGameplayEffect(instanceData.AppliedEffectHandle);
}
