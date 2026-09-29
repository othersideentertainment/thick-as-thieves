// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/StateTrees/Tasks/TATStateTreeTaskWaitForAppliedTags.h"

// ose
#include "Abilities/OSEAbilitySystemGlobals.h"

// ue
#include "AbilitySystemComponent.h"
#include "StateTreeExecutionContext.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskWaitForAppliedTags)

EStateTreeRunStatus FTATStateTreeTaskWaitForAppliedTags::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);

   if (instanceData.AIPawn == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskWaitForAppliedTags::EnterState failed since AIPawn is missing."));
      return EStateTreeRunStatus::Failed;
   }

   if (instanceData.TagsToWaitFor.IsEmpty())
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskWaitForAppliedTags::EnterState called with an empty 'TagsToWaitFor' on %s."),
         *instanceData.AIPawn->GetName());
      return EStateTreeRunStatus::Failed;
   }

   TObjectPtr<UAbilitySystemComponent> asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(instanceData.AIPawn);
   if (asc == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskWaitForAppliedTags::EnterState failed to retrieve ASC from %s."),
         *instanceData.AIPawn->GetName());
      return EStateTreeRunStatus::Failed;
   }

   // No need to make 'asc' a weak pointer - it should exist as it is what will use/call this lambda function.
   auto OnTagToWaitForChanged = [dataRef = context.GetInstanceDataStructRef(*this), asc](const FGameplayTag tag, int32 newTagCount) mutable
   {
      if (!dataRef.IsValid())
      {
         return;
      }

      if (FInstanceDataType* capturedInstanceData = dataRef.GetPtr())
      {
         // If we're still bound to the gameplay tag event (evident by the call to this lambda)
         // we should still have data for it.
         FInstanceTagDataType& tagData = capturedInstanceData->TagsToWaitForData.FindChecked(tag);

         const int32 oldTagCount = tagData.Count;

         bool isTagFinished = false;
         switch (capturedInstanceData->WaitForTypePerTag)
         {
            case ETATStateTreeTaskWaitForType::TagsApplied_Any:
               isTagFinished = ((newTagCount - oldTagCount) > 0);
               break;
            case ETATStateTreeTaskWaitForType::TagsApplied_First:
               isTagFinished = (oldTagCount == 0) || (newTagCount > 0);
               break;
            case ETATStateTreeTaskWaitForType::TagsRemoved_Any:
               isTagFinished = ((newTagCount - oldTagCount) < 0);
               break;
            case ETATStateTreeTaskWaitForType::TagsRemoved_All:
               isTagFinished = (oldTagCount > 0) || (newTagCount == 0);
               break;
         }

         if (isTagFinished)
         {
            asc->RegisterGameplayTagEvent(tag).Remove(tagData.Handle);
            capturedInstanceData->TagsToWaitForData.Remove(tag);

            if (capturedInstanceData->TagsToWaitForData.IsEmpty())
            {
               capturedInstanceData->IsFinished = true;
            }
         }
         else
         {
            tagData.Count = newTagCount;
         }
      }
   };

   const bool trackAnyCountChange =
      (instanceData.WaitForTypePerTag == ETATStateTreeTaskWaitForType::TagsApplied_Any) ||
      (instanceData.WaitForTypePerTag == ETATStateTreeTaskWaitForType::TagsRemoved_Any);
   const EGameplayTagEventType::Type tagEventType = 
      trackAnyCountChange ? EGameplayTagEventType::Type::AnyCountChange : EGameplayTagEventType::Type::NewOrRemoved;

   for (FGameplayTag tag : instanceData.TagsToWaitFor)
   {
      FInstanceTagDataType& tagData = instanceData.TagsToWaitForData.Emplace(tag);
      tagData.Count = asc->GetGameplayTagCount(tag);
      tagData.Handle = asc->RegisterGameplayTagEvent(tag, tagEventType).AddLambda(OnTagToWaitForChanged);
   }

   return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FTATStateTreeTaskWaitForAppliedTags::Tick(FStateTreeExecutionContext& context, const float deltaTime) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.IsFinished)
   {
      return EStateTreeRunStatus::Succeeded;
   }
   return EStateTreeRunStatus::Running;
}

void FTATStateTreeTaskWaitForAppliedTags::ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);

   if (instanceData.AIPawn == nullptr)
   {
      return;
   }

   TObjectPtr<UAbilitySystemComponent> asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(instanceData.AIPawn);
   if (asc == nullptr)
   {
      return;
   }

   // These should be unbound if the task finished naturally, but in the case that it was aborted
   // we need to make sure we clean these up.
   for (TPair<FGameplayTag, FInstanceTagDataType>& tagDataEntry : instanceData.TagsToWaitForData)
   {
      asc->RegisterGameplayTagEvent(tagDataEntry.Key).Remove(tagDataEntry.Value.Handle);
   }
}
