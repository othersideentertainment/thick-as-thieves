// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/StateTrees/Tasks/TATStateTreeTaskExtractStimDataFromEvent.h"

// ue
#include "StateTreeEvents.h"
#include "StateTreeExecutionContext.h"

// tat
#include "AI/TATAIController.h"
#include "AI/Perception/TATHearingTypes.h"
#include "AI/StateTrees/TATStateTreeEvents.h"
#include "AI/StateTrees/Targeting/TATStateTreeTargetingComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskExtractStimDataFromEvent)

EStateTreeRunStatus FTATStateTreeTaskExtractStimDataFromEvent::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);

   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskExtractStimDataFromEvent failed since AIController is missing."));
      return EStateTreeRunStatus::Failed;
   }

   const ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if (aiController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskExtractStimDataFromEvent failed since AIController is not a TATAIController."));
      return EStateTreeRunStatus::Failed;
   }

   UTATStateTreeTargetingComponent* targetingComponent = aiController->GetStateTreeTargetingComponent();
   if (targetingComponent == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskExtractStimDataFromEvent failed since StatetreeTargetingComponent is missing."));
      return EStateTreeRunStatus::Failed;
   }

   // Iterate through all the stim events and find the one that we most want to react to.
   const TArray<FStimInfo>& stims = targetingComponent->GetCurrentlyProcessedStimInfo();
   const FStimInfo* priorityStim = nullptr;
   for (const FStimInfo& stim : stims)
   {
      if (priorityStim == nullptr)
      {
         priorityStim = &stim;
      }
      else
      {
         priorityStim = &targetingComponent->DetermineHighestPriorityStim(*priorityStim, stim);
      }
   }

   if (priorityStim == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskExtractStimDataFromEvent failed as no stim events to process were found."));
      return EStateTreeRunStatus::Failed;
   }

   FGameplayTagContainer* behaviorTagContainer = instanceData.ResultBehaviors.GetMutablePtr<FGameplayTagContainer>(context);
   FStimInfo* resultStim = instanceData.ResultStim.GetMutablePtr<FStimInfo>(context);
   if (resultStim == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskExtractStimDataFromEvent failed to retrieve stim info."));
      return EStateTreeRunStatus::Failed;
   }
   if(behaviorTagContainer == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskExtractStimDataFromEvent failed to retrieve behavior tag container."));
      return EStateTreeRunStatus::Failed;
   }

   *resultStim = *priorityStim;

   // Hearing stims have extra data that tag certain behavior reactions, grab those tags
   FTATHearingEventStimSettings outStimData;
   if(targetingComponent->GetStimDataForTag(resultStim->Tag, outStimData))
   {
      *behaviorTagContainer = outStimData.Behaviors;
      if(double* outerSearchRadius = instanceData.OutOuterSearchRadius.GetMutablePtr<double>(context))
      {
         *outerSearchRadius = outStimData.OuterInvestigationRange;
      }
      if(double* innerSearchRadius = instanceData.OutInnerSearchRadius.GetMutablePtr<double>(context))
      {
         *innerSearchRadius = outStimData.OuterInvestigationRange;
      }
      if(bool* canStimLocationUpdate = instanceData.StimLocationCanUpdate.GetMutablePtr<bool>(context))
      {
         // If the stim query checks the location, then it's not possible for the location to change with the stim itself.
         *canStimLocationUpdate = EnumHasAnyFlags(
            static_cast<EStimDatabaseQueryBitmaskValues>(outStimData.QueryBitmask),
            EStimDatabaseQueryBitmaskValues::CheckLocation
         ) == false;
      }
   }
   targetingComponent->ConsumeCurrentProcessedStims();

   return EStateTreeRunStatus::Succeeded;
}
