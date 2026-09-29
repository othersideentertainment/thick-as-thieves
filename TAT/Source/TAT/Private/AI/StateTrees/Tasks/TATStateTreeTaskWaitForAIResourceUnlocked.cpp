// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/StateTrees/Tasks/TATStateTreeTaskWaitForAIResourceUnlocked.h"

// ue
#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskWaitForAIResourceUnlocked)

EStateTreeRunStatus FTATStateTreeTaskWaitForAIResourceUnlocked::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   static const FString contextString = TEXT("FTATStateTreeTaskWaitForAIResourceUnlocked::EnterState");
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   return _GetState(context, contextString);
}

EStateTreeRunStatus FTATStateTreeTaskWaitForAIResourceUnlocked::Tick(FStateTreeExecutionContext& context, const float deltaTime) const
{
   static const FString contextString = TEXT("FTATStateTreeTaskWaitForAIResourceUnlocked::Tick");
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   return _GetState(context, contextString);
}

EStateTreeRunStatus FTATStateTreeTaskWaitForAIResourceUnlocked::_GetState(const FStateTreeExecutionContext& context, const FString& contextString) const
{
   if (const IAIResourceInterface* resourceInterface = _GetOrFindResourceInterface(context, contextString))
   {
      return resourceInterface->IsResourceLocked() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
   }

   return EStateTreeRunStatus::Failed;
}

const IAIResourceInterface* FTATStateTreeTaskWaitForAIResourceUnlocked::_GetOrFindResourceInterface(const FStateTreeExecutionContext& context,
   const FString& contextString) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);

   if (const IAIResourceInterface* resourceInterface = instanceData.AIResourceInterface.Get())
   {
      return resourceInterface;
   }

   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("%s failed since AIController is missing."), *contextString);
      return nullptr;
   }

   UActorComponent* resourceComponent = instanceData.AIController->FindComponentByClass(instanceData.AIResourceComponentClass);
   if (resourceComponent != nullptr)
   {
      IAIResourceInterface* resourceInterface = Cast<IAIResourceInterface>(resourceComponent);
      check(resourceInterface);

      instanceData.AIResourceInterface = resourceInterface;
      return resourceInterface;
   }

   const APawn* aiPawn = instanceData.AIController->GetPawn();
   if (aiPawn != nullptr)
   {
      resourceComponent = aiPawn->FindComponentByClass(instanceData.AIResourceComponentClass);
      if (resourceComponent != nullptr)
      {
         IAIResourceInterface* resourceInterface = Cast<IAIResourceInterface>(resourceComponent);
         check(resourceInterface);

         instanceData.AIResourceInterface = resourceInterface;
         return resourceInterface;
      }
   }

   UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("%s failed to retrieve %s on %s or %s."), 
      *contextString, *instanceData.AIController.GetName(), *GetNameSafe(aiPawn));
   instanceData.AIResourceInterface = nullptr;
   return nullptr;
}
