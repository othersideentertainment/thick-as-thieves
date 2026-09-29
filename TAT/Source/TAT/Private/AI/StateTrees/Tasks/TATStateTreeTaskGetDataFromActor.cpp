// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/StateTrees/Tasks/TATStateTreeTaskGetDataFromActor.h"

// ue
#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskGetDataFromActor)

EStateTreeRunStatus FTATStateTreeTaskGetLocationFromActor::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);

   const AAIController* aiController = instanceData.AIController;
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskGetLocationFromActor failed since AIController is missing."));
      return EStateTreeRunStatus::Failed;
   }

   const APawn* aiPawn = instanceData.AIController->GetPawn();
   if (aiPawn == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskGetLocationFromActor failed to find a pawn possessed by AIController."));
      return EStateTreeRunStatus::Failed;
   }

   FVector* locationPtr = instanceData.OutLocation.GetMutablePtr<FVector>(context);
   if (locationPtr == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskGetLocationFromActor failed to find an out FVector param."));
      return EStateTreeRunStatus::Failed;
   }

   *locationPtr = aiPawn->GetActorLocation();
   return EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FTATStateTreeTaskGetRotationFromActor::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);

   const AAIController* aiController = instanceData.AIController;
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskGetRotationFromActor failed since AIController is missing."));
      return EStateTreeRunStatus::Failed;
   }

   const APawn* aiPawn = instanceData.AIController->GetPawn();
   if (aiPawn == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskGetRotationFromActor failed to find a pawn possessed by AIController."));
      return EStateTreeRunStatus::Failed;
   }

   FRotator* rotationPtr = instanceData.OutRotation.GetMutablePtr<FRotator>(context);
   if (rotationPtr == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskGetRotationFromActor failed to find an out FRotator param."));
      return EStateTreeRunStatus::Failed;
   }

   *rotationPtr = aiPawn->GetActorRotation();
   return EStateTreeRunStatus::Succeeded;
}
