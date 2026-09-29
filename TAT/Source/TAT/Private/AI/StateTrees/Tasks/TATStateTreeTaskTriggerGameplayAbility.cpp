// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/StateTrees/Tasks/TATStateTreeTaskTriggerGameplayAbility.h"

// ue
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "StateTreeExecutionContext.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"

// tat
#include "AI/TATAIController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskTriggerGameplayAbility)

UAbilitySystemComponent* GetAbilitySystemComponentFromController(AAIController* controller)
{
   if(controller == nullptr)
      return nullptr;
   const ATATAIController* aiController = Cast<ATATAIController>(controller);
   if (aiController == nullptr)
      return nullptr;
   if(const IAbilitySystemInterface* abilitySystemInterface = Cast<IAbilitySystemInterface>(aiController->GetPawn()))
      return abilitySystemInterface->GetAbilitySystemComponent();

   return nullptr;
}

// FTATStateTreeTaskTriggerGameplayAbilityByClass and FTATStateTreeTaskTriggerGameplayAbility are almost identical - I'm not sure how to get
// inheritance to work with the different FInstanceDataTypes (Maybe subclass?) For now I'll just keep the duplicate code and move on.

EStateTreeRunStatus FTATStateTreeTaskTriggerGameplayAbilityByClass::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);
   UAbilitySystemComponent* asc = GetAbilitySystemComponentFromController(instanceData.AIController);
   if(asc == nullptr)
   {
      return EStateTreeRunStatus::Failed;
   }
   if(const FGameplayAbilitySpec* abilitySpec = asc->FindAbilitySpecFromClass(instanceData.AbilityClassToTrigger))
   {
      instanceData.BoundDelegateHandle = asc->OnAbilityEnded.AddLambda(
         [dataRef = context.GetInstanceDataStructRef(*this), handle = abilitySpec->Handle](const FAbilityEndedData& abilityEndedData) mutable 
      {
         if(dataRef.IsValid() == false)
         {
            return;
         }
         if(handle!= abilityEndedData.AbilitySpecHandle)
         {
            return;
         }
         if (FInstanceDataType* capturedInstanceData = dataRef.GetPtr())
         {
            capturedInstanceData->IsFinished = true;
         }
      });
      if(asc->TryActivateAbility(abilitySpec->Handle))
      {
         instanceData.ActivatedAbilitySpecHandle = abilitySpec->Handle;
         return EStateTreeRunStatus::Running;
      }
   }
   return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FTATStateTreeTaskTriggerGameplayAbilityByClass::Tick(FStateTreeExecutionContext& context, const float deltaTime) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if(instanceData.IsFinished)
      return EStateTreeRunStatus::Succeeded;
   return EStateTreeRunStatus::Running;
}

void FTATStateTreeTaskTriggerGameplayAbilityByClass::ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   UAbilitySystemComponent* asc = GetAbilitySystemComponentFromController(instanceData.AIController);
   if(asc == nullptr)
   {
      return;
   }
   asc->OnAbilityEnded.Remove(instanceData.BoundDelegateHandle);
   if(instanceData.EndAbilityOnTaskExit && instanceData.ActivatedAbilitySpecHandle.IsValid())
   {
      asc->CancelAbilityHandle(instanceData.ActivatedAbilitySpecHandle);
   }
}

EStateTreeRunStatus FTATStateTreeTaskTriggerGameplayAbility::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);
   UAbilitySystemComponent* asc = GetAbilitySystemComponentFromController(instanceData.AIController);
   if(asc == nullptr)
   {
      return EStateTreeRunStatus::Failed;
   }
   if(const FGameplayAbilitySpec* abilitySpec = asc->FindAbilitySpecFromInputID(static_cast<int32>(instanceData.AbilityInputToTrigger)))
   {
      TStateTreeInstanceDataStructRef<FInstanceDataType> dataRef = context.GetInstanceDataStructRef(*this);
      instanceData.BoundDelegateHandle = asc->OnAbilityEnded.AddLambda(
         [dataRef = context.GetInstanceDataStructRef(*this), handle = abilitySpec->Handle](const FAbilityEndedData& abilityEndedData) mutable 
      {
         if(dataRef.IsValid() == false)
         {
            return;
         }
         if(handle != abilityEndedData.AbilitySpecHandle)
         {
            return;
         }
         if (FInstanceDataType* capturedInstanceData = dataRef.GetPtr())
         {
            capturedInstanceData->IsFinished = true;
         }
      });
      if(asc->TryActivateAbility(abilitySpec->Handle))
      {
         instanceData.ActivatedAbilitySpecHandle = abilitySpec->Handle;
         return EStateTreeRunStatus::Running;
      }
   }
   return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FTATStateTreeTaskTriggerGameplayAbility::Tick(FStateTreeExecutionContext& context, const float deltaTime) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if(instanceData.IsFinished)
      return EStateTreeRunStatus::Succeeded;
   return EStateTreeRunStatus::Running;
}

void FTATStateTreeTaskTriggerGameplayAbility::ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   UAbilitySystemComponent* asc = GetAbilitySystemComponentFromController(instanceData.AIController);
   if(asc == nullptr)
   {
      return;
   }
   asc->OnAbilityEnded.Remove(instanceData.BoundDelegateHandle);
   if (instanceData.EndAbilityOnTaskExit && instanceData.ActivatedAbilitySpecHandle.IsValid())
   {
      asc->CancelAbilityHandle(instanceData.ActivatedAbilitySpecHandle);
   }
}

EStateTreeRunStatus FTATStateTreeTaskTriggerGameplayAbilityByEvent::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);

   UOSEAbilitySystemComponent* asc = Cast<UOSEAbilitySystemComponent>(GetAbilitySystemComponentFromController(instanceData.AIController));
   if (asc == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, 
         TEXT("FTATStateTreeTaskTriggerGameplayAbilityByEvent failed to retrieve ability system component."));
      return EStateTreeRunStatus::Failed;
   }

   // Can we find an ability that can activate based on this event tag?
   const FGameplayAbilitySpec* abilitySpec = asc->FindFirstActivatableAbilityForGameplayEvent(instanceData.EventData);
   if (abilitySpec == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error,
         TEXT("FTATStateTreeTaskTriggerGameplayAbilityByEvent failed to find an activatable event-driven gameplay ability linked with %s."),
         *instanceData.EventTag.ToString());
      return EStateTreeRunStatus::Failed;
   }

   // Track when the ability ends so we can end this task.
   instanceData.BoundDelegateHandle = asc->OnAbilityEnded.AddLambda(
      [dataRef = context.GetInstanceDataStructRef(*this), handle = abilitySpec->Handle](const FAbilityEndedData& abilityEndedData) mutable
   {
      if (dataRef.IsValid() == false)
      {
         return;
      }
      if (handle != abilityEndedData.AbilitySpecHandle)
      {
         return;
      }
      if (FInstanceDataType* capturedInstanceData = dataRef.GetPtr())
      {
         capturedInstanceData->IsFinished = true;
      }
   });

   const int32 activationCount = asc->HandleGameplayEvent(instanceData.EventTag, &instanceData.EventData);
   if (activationCount == 0)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error,
         TEXT("FTATStateTreeTaskTriggerGameplayAbilityByEvent failed to activate a gameplay ability linked with %s."),
         *instanceData.EventTag.ToString());
      return EStateTreeRunStatus::Failed;
   }

   instanceData.ActivatedAbilitySpecHandle = abilitySpec->Handle;
   if (!ensure(instanceData.ActivatedAbilitySpecHandle.IsValid()))
   {
      return EStateTreeRunStatus::Failed;
   }

   return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FTATStateTreeTaskTriggerGameplayAbilityByEvent::Tick(FStateTreeExecutionContext& context, const float deltaTime) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.IsFinished)
   {
      return EStateTreeRunStatus::Succeeded;
   }
   return EStateTreeRunStatus::Running;
}

void FTATStateTreeTaskTriggerGameplayAbilityByEvent::ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);

   UAbilitySystemComponent* asc = GetAbilitySystemComponentFromController(instanceData.AIController);
   if (asc == nullptr)
   {
      return;
   }

   asc->OnAbilityEnded.Remove(instanceData.BoundDelegateHandle);

   if (instanceData.EndAbilityOnTaskExit && instanceData.ActivatedAbilitySpecHandle.IsValid())
   {
      asc->CancelAbilityHandle(instanceData.ActivatedAbilitySpecHandle);
   }
}
