// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/StateTrees/Tasks/Reactions/TATStateTreeTaskRegisterForRole.h"

// ue
#include "AIController.h"
#include "StateTreeExecutionContext.h"

// tat
#include "AI/Reactions/TATAIReactionCoordinator.h"
#include "AI/Reactions/TATAIReactionSettings.h"
#include "AI/StateTrees/TATStateTreeEvents.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskRegisterForRole)

FTATStateTreeTaskRegisterForRole::FTATStateTreeTaskRegisterForRole()
{
   // No tick needed.
   bShouldCallTick = false;

   // No need to update bound properties after enter state.
   bShouldCopyBoundPropertiesOnTick = false;
   bShouldCopyBoundPropertiesOnExitState = false;

   // Prevent re-entering when an active child state transitions.
   bShouldStateChangeOnReselect = false;
}

void FTATStateTreeTaskRegisterForRole::AttemptToSetTargetFromContext(FStateTreeExecutionContext& context,
                                                                     const FInstanceDataType& instanceData,
                                                                     const ATATCharacterAIBase* aiCharacter,
                                                                     FTATAIReactionTarget& target)
{
   const FStimInfo* stimPtr = instanceData.ReactingToStim.GetMutablePtr<FStimInfo>(context);
   if (stimPtr != nullptr)
   {
      target = FTATAIReactionTarget::GenerateForStim(aiCharacter, *stimPtr);
   }
   AActor** actorPtr = instanceData.ReactingToStim.GetMutablePtr<AActor*>(context);
   if(actorPtr != nullptr)
   {
      const AActor* targetActor = *actorPtr;
      target = FTATAIReactionTarget::GenerateForActor(targetActor);
   }
}

EStateTreeRunStatus FTATStateTreeTaskRegisterForRole::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);

   const AAIController* aiController = instanceData.AIController;
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskRegisterForRole::EnterState failed since AIController is missing."));
      return EStateTreeRunStatus::Failed;
   }

   ATATCharacterAIBase* aiCharacter = Cast<ATATCharacterAIBase>(instanceData.AIController->GetPawn());
   if (aiCharacter == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskRegisterForRole::EnterState failed since AIController is not a TATAIController"));
      return EStateTreeRunStatus::Failed;
   }

   FGameplayTag* outRoleTag = instanceData.ResultRegisteredRoleTag.GetMutablePtr<FGameplayTag>(context);
   if (outRoleTag == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskRegisterForRole::EnterState failed to find an out role tag param."));
      return EStateTreeRunStatus::Failed;
   }

   UTATAIReactionCoordinatorSubsystem* reactionCoordinatorSubsystem = aiController->GetWorld()->GetSubsystem<UTATAIReactionCoordinatorSubsystem>();
   if(reactionCoordinatorSubsystem == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskRegisterForRole::EnterState UTATAIReactionCoordinatorSubsystem is null"));
      return EStateTreeRunStatus::Failed;
   }
   
   FTATAIReactionTarget target = FTATAIReactionTarget();
   AttemptToSetTargetFromContext(context, instanceData, aiCharacter, target);
   if(target.IsValid() == false)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskRegisterForRole::EnterState failed to retrieve eiteher a stim info or an actor"));
      return EStateTreeRunStatus::Failed;
   }
   
   if (instanceData.AllowToRunIfNoEventConfigExists)
   {
      if (UTATAIReactionSettings::Get().HasEventConfigForTarget(target) == false)
      {
         // It's possible there may be no authored reaction target/event config for this target. If so,
         // allow the task to run without fail so it's child states can execute.
         return EStateTreeRunStatus::Running;
      }
   }
   
   if (reactionCoordinatorSubsystem->RegisterAIForTarget(aiCharacter, target, *outRoleTag))
   {
      return EStateTreeRunStatus::Running;
   }
   return EStateTreeRunStatus::Failed;
}

void FTATStateTreeTaskRegisterForRole::ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);

   const AAIController* aiController = instanceData.AIController;
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskRegisterForRole::ExitState failed since AIController is missing."));
      return;
   }

   ATATCharacterAIBase* aiCharacter = Cast<ATATCharacterAIBase>(instanceData.AIController->GetPawn());
   if (aiCharacter == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskRegisterForRole::ExitState failed since ATATCharacterAIBase could not be retrieved."));
      return;
   }

   FGameplayTag* outRoleTag = instanceData.ResultRegisteredRoleTag.GetMutablePtr<FGameplayTag>(context);
   if (outRoleTag == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskRegisterForRole::ExitState failed to find an out role tag param."));
      return;
   }

   UTATAIReactionCoordinatorSubsystem* reactionCoordinatorSubsystem = aiController->GetWorld()->GetSubsystem<UTATAIReactionCoordinatorSubsystem>();
   if(reactionCoordinatorSubsystem == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskRegisterForRole::ExitState UTATAIReactionCoordinatorSubsystem is null"));
      return;
   }
   
   FTATAIReactionTarget target = FTATAIReactionTarget();
   AttemptToSetTargetFromContext(context, instanceData, aiCharacter, target);
   if(target.IsValid())
   {
      if (reactionCoordinatorSubsystem->UnregisterAIForTarget(aiCharacter, target))
      {
         *outRoleTag = FGameplayTag::EmptyTag;
      }
   }
}
