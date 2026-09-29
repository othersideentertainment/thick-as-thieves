// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Tasks/Lockdown/TATStateTreeLockdownExtractSearchTarget.h"

// tat
#include "AI/Coordinators/TATLockdownCoordinator.h"
#include "AI/StateTrees/TATStateTreeEvents.h"

// ue
#include "StateTreeExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeLockdownExtractSearchTarget)

EStateTreeRunStatus FTATStateTreeLockdownExtractSearchTarget::EnterState(FStateTreeExecutionContext& context,
                                                                         const FStateTreeTransitionResult& transition) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeLockdownExtractSearchTarget failed since AIController is missing."));
      return EStateTreeRunStatus::Failed;
   }

   UTATLockdownCoordinator* lockdownCoordinator = instanceData.AIController->GetWorld()->GetSubsystem<UTATLockdownCoordinator>();
   if (lockdownCoordinator == nullptr)
   {
      return EStateTreeRunStatus::Failed;
   }

   ATATCharacterAIBase* character = instanceData.AIController->GetPawn<ATATCharacterAIBase>();
   instanceData.LocationToSearch = lockdownCoordinator->GetNextLockdownLocationToSearch(character);
   if (instanceData.LocationToSearch == nullptr)
   {
      if (lockdownCoordinator->TrySetCharacterTurningOffAlarm(character))
      {
         context.SendEvent(TAG_StateTreeEvent_LockdownNewAlarmGuard);
      }
      else
      {
         context.SendEvent(TAG_StateTreeEvent_LockdownHoldPosition);
      }
      return EStateTreeRunStatus::Failed;
   }
   
   instanceData.LocationVectorToSearch = instanceData.LocationToSearch->GetActorLocation();
   return EStateTreeRunStatus::Running;
}

void FTATStateTreeLockdownExtractSearchTarget::ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
 
}

FTATStateTreeLockdown::FTATStateTreeLockdown()
{
   bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FTATStateTreeLockdown::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   return EStateTreeRunStatus::Running;
}

void FTATStateTreeLockdown::ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeLockdown failed since AIController is missing."));
      return;
   }

   UTATLockdownCoordinator* lockdownCoordinator = instanceData.AIController->GetWorld()->GetSubsystem<UTATLockdownCoordinator>();
   if (lockdownCoordinator == nullptr)
   {
      return;
   }

   ATATCharacterAIBase* character = instanceData.AIController->GetPawn<ATATCharacterAIBase>();
   lockdownCoordinator->LeaveLockdown(character);
}

EStateTreeRunStatus FTATStateTreeLockdownGetAlarmStation::EnterState(FStateTreeExecutionContext& context,
                                                                     const FStateTreeTransitionResult& transition) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeLockdownGetAlarmStation failed since AIController is missing."));
      return EStateTreeRunStatus::Failed;
   }

   UTATLockdownCoordinator* lockdownCoordinator = instanceData.AIController->GetWorld()->GetSubsystem<UTATLockdownCoordinator>();
   if (lockdownCoordinator == nullptr)
   {
      return EStateTreeRunStatus::Failed;
   }
   
   instanceData.AlarmStation = lockdownCoordinator->GetAlarmStationForCharacter(instanceData.AIController->GetPawn<ATATCharacterAIBase>());
   return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FTATStateTreeLockdownFinishSearching::EnterState(FStateTreeExecutionContext& context,
   const FStateTreeTransitionResult& transition) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeLockdownFinishSearching failed since AIController is missing."));
      return EStateTreeRunStatus::Failed;
   }

   UTATLockdownCoordinator* lockdownCoordinator = instanceData.AIController->GetWorld()->GetSubsystem<UTATLockdownCoordinator>();
   if (lockdownCoordinator == nullptr)
   {
      return EStateTreeRunStatus::Failed;
   }

   ATATCharacterAIBase* character = instanceData.AIController->GetPawn<ATATCharacterAIBase>();
   lockdownCoordinator->FinishSearchingLocation(character);
   return EStateTreeRunStatus::Succeeded;
}
