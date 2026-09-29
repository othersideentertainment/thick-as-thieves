// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/StateTrees/Conditionals/TATStateTreeConditionActorHasPathToOwner.h"

// ue
#include "StateTreeExecutionContext.h"

// ose
#include "AI/TATAIController.h"
#include "Math/OSEMathFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeConditionActorHasPathToOwner)

bool FTATStateTreeConditionActorHasPathToOwner::TestCondition(FStateTreeExecutionContext& context) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if(instanceData.Target == nullptr)
      return false;

   const ATATAIController* aiController = Cast<ATATAIController>(instanceData.Controller);
   if(aiController == nullptr)
      return false;

   UTATKnowledgeComponent* knowledgeComponent = aiController->GetTATKnowledgeComponent();
   if(knowledgeComponent == nullptr)
      return false;

   const FTATActorKnowledge* actorKnowledge = knowledgeComponent->GetActorKnowledge(instanceData.Target);
   if(actorKnowledge == nullptr)
      return false;
   const bool anyPath = actorKnowledge->GetHasAnyPathToLastKnownLocation();
   const bool fullPath = actorKnowledge->GetHasFullPathToLastKnownLocation();

   if(instanceData.RequireAnyPath)
   {
      return anyPath;
   }
   if(instanceData.RequireFullPath)
   {
      return fullPath;
   }
   return instanceData.RequireNoPath && anyPath == false && fullPath == false;
}
