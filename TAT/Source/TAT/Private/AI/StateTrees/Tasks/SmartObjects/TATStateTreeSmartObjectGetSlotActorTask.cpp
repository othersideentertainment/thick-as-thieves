// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Tasks/SmartObjects/TATStateTreeSmartObjectGetSlotActorTask.h"

// tat
#include "AI/StateTrees/Tasks/SmartObjects/TATStateTreeSmartObjectTypes.h"

// ue
#include "SmartObjectDefinition.h"
#include "SmartObjectRuntime.h"
#include "SmartObjectSubsystem.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "Annotations/SmartObjectSlotLinkAnnotation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeSmartObjectGetSlotActorTask)

FTATStateTreeSmartObjectGetSlotActorTask::FTATStateTreeSmartObjectGetSlotActorTask()
{
   // No tick needed.
   bShouldCallTick = false;
   // No need to update bound properties after enter state.
   bShouldCopyBoundPropertiesOnTick = false;
   bShouldCopyBoundPropertiesOnExitState = false;
}

bool FTATStateTreeSmartObjectGetSlotActorTask::Link(FStateTreeLinker& linker)
{
   linker.LinkExternalData(_SmartObjectSubsystemHandle);
   return true;
}

EStateTreeRunStatus FTATStateTreeSmartObjectGetSlotActorTask::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   const USmartObjectSubsystem& smartObjectSubsystem = context.GetExternalData(_SmartObjectSubsystemHandle);
   FInstanceDataType& instanceData = context.GetInstanceData(*this);

   if (instanceData.TargetSlot.IsValid() == false)
   {
      UE_VLOG_UELOG(context.GetOwner(), LogStateTree, Error, TEXT("[TATStateTreeSmartObjectGetSlotActorTask] Expected valid TargetSlot handle."));
      return EStateTreeRunStatus::Failed;
   }

   instanceData.ResultActor = nullptr;
	
   const FSmartObjectSlotView slotView = smartObjectSubsystem.GetSlotView(instanceData.TargetSlot);
   if (slotView.IsValid())
   {
      if (const FTATSmartObjectInteractionSlotUserData* userData = slotView.GetStateDataPtr<FTATSmartObjectInteractionSlotUserData>())
      {
         instanceData.ResultActor = userData->UserActor.Get();
      }
   }
	
   if (_bFailIfNotFound && IsValid(instanceData.ResultActor) == false)
   {
      return EStateTreeRunStatus::Failed;
   }

   return EStateTreeRunStatus::Running;  
}
