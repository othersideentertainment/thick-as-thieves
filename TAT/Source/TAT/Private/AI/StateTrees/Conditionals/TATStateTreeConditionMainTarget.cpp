// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Conditionals/TATStateTreeConditionMainTarget.h"

// ue
#include "StateTreeExecutionContext.h"

// tat
#include "AI/TATAIController.h"
#include "AI/Perception/TATHearingTypes.h"
#include "AI/StateTrees/TATStateTreeEvents.h"
#include "AI/StateTrees/Targeting/TATStateTreeTargetingComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeConditionMainTarget)

namespace TATStateTreeConditionMainTarget
{
   const FTATAITargetingEvent_TargetChanged* GetEventFromContext(const FStateTreeExecutionContext& context)
   {
      const TConstArrayView<FStateTreeSharedEvent> events = context.GetEventsToProcessView();
      for (const FStateTreeSharedEvent& event : events)
      {
         if(event->Tag != TAG_StateTreeEvent_TargetChange)
            continue;
         return event->Payload.GetPtr<FTATAITargetingEvent_TargetChanged>();
      }
      return nullptr;
   }
}

bool FTATStateTreeConditionMainTargetingGroupMatches::TestCondition(FStateTreeExecutionContext& context) const
{
   const FTATAITargetingEvent_TargetChanged* targetChangeEvent = TATStateTreeConditionMainTarget::GetEventFromContext(context);
   if(targetChangeEvent == nullptr)
      return false;

   const FInstanceDataType& instanceData = context.GetInstanceData(*this);

   const bool tagMatches = instanceData.MatchExact
      ? (instanceData.RequiredTag.MatchesTagExact(targetChangeEvent->TargetingTag))
      : (instanceData.RequiredTag.MatchesTag(targetChangeEvent->TargetingTag));
   return (tagMatches == instanceData.ShouldMatch);
}

bool FTATStateTreeConditionMainTargetMatchesDetection::TestCondition(FStateTreeExecutionContext& context) const
{
   const FTATAITargetingEvent_TargetChanged* targetChangeEvent = TATStateTreeConditionMainTarget::GetEventFromContext(context);
   if(targetChangeEvent == nullptr)
      return false;
   
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeConditionMainTargetMatchesDetection failed since AIController is missing."));
      return false;
   }

   const ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if (aiController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeConditionMainTargetMatchesDetection failed since AIController is not a TATAIController"));
      return false;
   }

   const FTATActorKnowledge* knowledge = aiController->GetTATKnowledgeComponent()->GetActorKnowledge(targetChangeEvent->Target);
   if(knowledge == nullptr)
   {
      return false;
   }
   
   return UOSEMathFunctionLibrary::CompareInts(
      static_cast<int>(knowledge->GetDetectionState()),
      static_cast<int>(instanceData.RequiredDetectionState),
      instanceData.ComparisonMethod);
}

bool FTATStateTreeConditionMainTargetMatchesVisibility::TestCondition(FStateTreeExecutionContext& context) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeConditionMainTargetMatchesVisibility failed since AIController is missing."));
      return false;
   }

   const ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if (aiController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeConditionMainTargetMatchesVisibility failed since AIController is not a TATAIController"));
      return false;
   }

   const FTATActorKnowledge* knowledge = aiController->GetTATKnowledgeComponent()->GetActorKnowledge(instanceData.Target);
   if(knowledge == nullptr)
   {
      return false;
   }
   return knowledge->GetIsVisible() == instanceData.RequiredVisibility;
}

bool FTATStateTreeConditionMainTargetMatchesActor::TestCondition(FStateTreeExecutionContext& context) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   const FTATAITargetingEvent_TargetChanged* targetChangeEvent = TATStateTreeConditionMainTarget::GetEventFromContext(context);
   if (targetChangeEvent == nullptr)
   {
      return false;
   }

   const bool doTargetsMatch = (instanceData.Target == targetChangeEvent->Target);
   return (doTargetsMatch == !(instanceData.Invert));
}

bool FTATStateTreeConditionHasTargetInGroup::TestCondition(FStateTreeExecutionContext& context) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeConditionHasTargetInGroup failed since AIController is missing."));
      return false;
   }

   const ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if (aiController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeConditionHasTargetInGroup failed since AIController is not a TATAIController"));
      return false;
   }
   if(const UTATStateTreeTargetingComponent* targetingComponent = aiController->GetStateTreeTargetingComponent())
   {
      const bool hasTargetInGroup = (targetingComponent->GetBestTargetForTargetingGroup(instanceData.TargetingGroup) != nullptr);
      return hasTargetInGroup == !instanceData.Invert;
   }
   return false;
}
