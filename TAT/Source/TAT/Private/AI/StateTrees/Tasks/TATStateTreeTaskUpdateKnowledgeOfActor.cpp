// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Tasks/TATStateTreeTaskUpdateKnowledgeOfActor.h"

// ue
#include "StateTreeExecutionContext.h"
// tat
#include "AI/TATAIController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskUpdateKnowledgeOfActor)

EStateTreeRunStatus FTATStateTreeTaskUpdateKnowledgeOfActor::SetLastKnownLocation(FStateTreeExecutionContext& context, bool failIfNotSet) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskUpdateKnowledgeOfActor failed since AIController is missing."));
      return EStateTreeRunStatus::Failed;
   }

   const ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if (aiController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskUpdateKnowledgeOfActor failed since AIController is not a TATAIController"));
      return EStateTreeRunStatus::Failed;
   }

   FVector* lastKnownLocation = instanceData.ResultLastKnownLocation.GetMutablePtr<FVector>(context);
   if(lastKnownLocation == nullptr)
      return EStateTreeRunStatus::Failed;
   
   // Don't set lastKnownLocation to InvalidLocation by default, if we do that then in the instance where we _previously_ had a valid location
   // but now don't (because the target expired naturally) we would end up wanting to search the location "last known", instead of returning EStateTreeRunStatus::Failed
   // which would exit the child states incorrectly.
   
   if(UTATKnowledgeComponent* knowledgeComponent = aiController->GetTATKnowledgeComponent())
   {
      if(const FTATActorKnowledge* knowledge = knowledgeComponent->GetActorKnowledge(instanceData.Target))
      {
         *lastKnownLocation = knowledge->GetLastKnownLocation();
         return ShouldTrackLocation ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
      }
   }
   
   if(failIfNotSet == false)
   {
      // If the target actor knowledge is no longer set, don't update but also keep running as this is a valid state for a NPC who's fully lost the target
      // instead we are going to retain the "last known location" in the state and search the area
      return ShouldTrackLocation ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
   }
   UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskUpdateKnowledgeOfActor failed because can't find knowledge of %s"), *GetNameSafe(instanceData.Target));
   return EStateTreeRunStatus::Failed;
}


EStateTreeRunStatus FTATStateTreeTaskUpdateKnowledgeOfActor::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   // We want to fail if the position is not set on Enter, this likely means we have no detection data of the target, as such we wouldn't have a valid location
   // to move to.
   return SetLastKnownLocation(context, true);
}

EStateTreeRunStatus FTATStateTreeTaskUpdateKnowledgeOfActor::Tick(FStateTreeExecutionContext& context, const float deltaTime) const
{
   // Here we want to not fail if not set, because the position was set on EnterState correctly, so we have a valid location we should continue to use (or update if we 
   // have a new valid location to search).
   return SetLastKnownLocation(context, false);
}
