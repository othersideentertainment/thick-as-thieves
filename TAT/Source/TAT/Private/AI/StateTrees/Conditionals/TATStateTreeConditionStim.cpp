// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Conditionals/TATStateTreeConditionStim.h"

// ue
#include "StateTreeExecutionContext.h"

// tat
#include "AI/TATAIController.h"
#include "AI/Perception/TATHearingTypes.h"
#include "AI/StateTrees/TATStateTreeEvents.h"
#include "AI/StateTrees/Targeting/TATStateTreeTargetingComponent.h"
#include "Environment/TATPrivateSpaceCharacterComponent.h"
#include "Environment/TATPrivateSpaceCharacterInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeConditionStim)

namespace TATStateTreeConditionStim
{
   const FTATTargetingEvent_Stim* GetEventFromContext(const FStateTreeExecutionContext& context)
   {
      const TConstArrayView<FStateTreeSharedEvent> events = context.GetEventsToProcessView();
      for (const FStateTreeSharedEvent& event : events)
      {
         if(event->Tag != TAG_StateTreeEvent_StimChange)
            continue;
         return event->Payload.GetPtr<FTATTargetingEvent_Stim>();
      }
      return nullptr;
   }
   bool HandleMatching(const FStimInfo& stimInfo, const FTATStateTreeConditionStimInstanceData& stimMatchingRules)
   {
      if(stimMatchingRules.CheckStimSeverityMatches)
      {
         if(stimInfo.Severity != stimMatchingRules.TargetStimSeverity)
            return false;
      }
      if(stimMatchingRules.CheckStimTagMatches)
      {
         if(stimInfo.Tag != stimMatchingRules.StimInfoRow.RowName)
            return false;
      }

      if(stimMatchingRules.CheckStimTypeMatches)
      {
         if(stimInfo.Type != stimMatchingRules.StimType)
            return false;
      }
      if(stimMatchingRules.CheckStimInstigatorHasTags)
      {
         if(const IGameplayTagAssetInterface* tagAssetInterface = Cast<IGameplayTagAssetInterface>(stimInfo.Instigator.Get()))
         {
            if(tagAssetInterface->HasAllMatchingGameplayTags(stimMatchingRules.StimInstigatorTags) == false)
               return false;
         }
      }
      return true;
   }
}

bool FTATStateTreeConditionStimEvent::TestCondition(FStateTreeExecutionContext& context) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   const FTATTargetingEvent_Stim* targetChangeEvent = TATStateTreeConditionStim::GetEventFromContext(context);
   if(targetChangeEvent == nullptr)
      return false;
   return TATStateTreeConditionStim::HandleMatching(targetChangeEvent->Stim, instanceData);
}

bool FTATStateTreeConditionStim::TestCondition(FStateTreeExecutionContext& context) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   const FStimInfo& stimInfo = instanceData.StimInfo;
   return TATStateTreeConditionStim::HandleMatching(stimInfo, instanceData);
}

bool FTATStateTreeConditionStimInstigatorMatchesAttitude::TestCondition(FStateTreeExecutionContext& context) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   const FStimInfo& stimInfo = instanceData.StimInfo;
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeConditionStimInstigatorMatchesAttitude failed since AIController is missing."));
      return false;
   }

   const ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if (aiController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeConditionStimInstigatorMatchesAttitude failed since AIController is not a TATAIController"));
      return false;
   }
   const EOSETeamAttitude attitude = UOSETeamFunctionLibrary::GetTeamAttitude(aiController->GetPawn(), stimInfo.Instigator.Get());
   return UOSEMathFunctionLibrary::CompareInts(
      static_cast<int>(attitude),
      static_cast<int>(instanceData.RequiredTeamAttitude),
      instanceData.ComparisonMethod);
}

bool FTATStateTreeConditionStimFromSharedPrivateZoneActor::TestCondition(FStateTreeExecutionContext& context) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   const FStimInfo& stimInfo = instanceData.StimInfo;
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeConditionStimInstigatorMatchesAttitude failed since AIController is missing."));
      return false;
   }

   const ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if (aiController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeConditionStimInstigatorMatchesAttitude failed since AIController is not a TATAIController"));
      return false;
   }
   const IGameplayTagAssetInterface* gameplayTagAssetInterfaceInstigator = Cast<IGameplayTagAssetInterface>(stimInfo.Instigator.Get());
   ITATPrivateSpaceCharacterInterface* aiControlledPawn = Cast<ITATPrivateSpaceCharacterInterface>(aiController->GetPawn());
   if(aiControlledPawn == nullptr || gameplayTagAssetInterfaceInstigator == nullptr)
   {
      return false;
   }

   const UTATPrivateSpaceCharacterComponent* aiControlledPawnActorComponent = aiControlledPawn->GetPrivateSpaceCharacterComponent();
   if(aiControlledPawnActorComponent == nullptr)
   {
      return false;
   }
   const FGameplayTagContainer aiControlledPawnAllowedTags = aiControlledPawnActorComponent->AuthorityGetAllAllowedPrivateZone();

   const bool actualValue = gameplayTagAssetInterfaceInstigator->HasAnyMatchingGameplayTags(aiControlledPawnAllowedTags);
   // If the stim instigator has the same private zone tags as the ai agent, match
   return instanceData.Invert ? actualValue == false : actualValue;
}
