// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Tasks/Combat/TATStateTreeTaskSetCombatTarget.h"

// ue
#include "StateTreeExecutionContext.h"

// tat 
#include "AI/TATAIController.h"
#include "AI/StateTrees/Interfaces/TATStateTreeCombatTargetInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskSetCombatTarget)

FTATStateTreeTaskSetCombatTarget::FTATStateTreeTaskSetCombatTarget()
{
   bShouldStateChangeOnReselect = false;
   bShouldCallTick = false;
}

EStateTreeRunStatus FTATStateTreeTaskSetCombatTarget::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskSetCombatTarget failed since AIController is missing."));
      return EStateTreeRunStatus::Failed;
   }

   ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if (aiController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskSetCombatTarget failed since AIController is not a TATAIController"));
      return EStateTreeRunStatus::Failed;
   }
   // Tell the AI Controller who we are targeting within the state tree
   aiController->OnEnterTargetingActorForStateTree(instanceData.Target);

   AOSECharacterBase* aiCharacter = aiController->GetOSECharacter();   
   if (aiCharacter == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskSetCombatTarget failed since Character is not a OSECharacterBase"));
      return EStateTreeRunStatus::Failed;
   }
   

   ITATStateTreeCombatTargetInterface* stateTreeTargetInterface = Cast<ITATStateTreeCombatTargetInterface>(instanceData.Target);
   if(stateTreeTargetInterface == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Warning, TEXT("FTATStateTreeTaskSetCombatTarget failed since Target is not a ITATStateTreeTargetInterface"));
      return EStateTreeRunStatus::Running;
   }
   stateTreeTargetInterface->OnEnterTargetedByStateTree(aiCharacter);
   return EStateTreeRunStatus::Running;
}

void FTATStateTreeTaskSetCombatTarget::ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   Super::ExitState(context, transition);
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskSetCombatTarget failed since AIController is missing."));
      return;
   }

   ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if (aiController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskSetCombatTarget failed since AIController is not a TATAIController"));
      return;
   }
   // Tell the AI Controller to reset who we are targeting.
   aiController->OnExitTargetingActorForStateTree();

   AOSECharacterBase* aiCharacter = aiController->GetOSECharacter();   
   if (aiCharacter == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskSetCombatTarget failed since Character is not a OSECharacterBase"));
      return;
   }

   ITATStateTreeCombatTargetInterface* stateTreeTargetInterface = Cast<ITATStateTreeCombatTargetInterface>(instanceData.Target);
   if(stateTreeTargetInterface == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskSetCombatTarget failed since Target is not a ITATStateTreeTargetInterface"));
      return;
   }
   stateTreeTargetInterface->OnExitTargetedByStateTree(aiCharacter);
}
