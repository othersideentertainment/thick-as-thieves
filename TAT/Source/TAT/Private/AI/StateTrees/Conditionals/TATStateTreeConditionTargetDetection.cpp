// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Conditionals/TATStateTreeConditionTargetDetection.h"

// ue
#include "StateTreeExecutionContext.h"

// tat
#include "AI/TATAIController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeConditionTargetDetection)

bool FTATStateTreeConditionTargetDetection::TestCondition(FStateTreeExecutionContext& context) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if(instanceData.Target == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeConditionTargetDetection failed since Target is missing."));
      return false;
   }
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeConditionTargetDetection failed since AIController is missing."));
      return false;
   }

   const ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if (aiController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeConditionTargetDetection failed since AIController is not a TATAIController"));
      return false;
   }

   const FTATActorKnowledge* knowledge = aiController->GetTATKnowledgeComponent()->GetActorKnowledge(instanceData.Target);
   if(knowledge == nullptr)
   {
      return false;
   }
   
   return UOSEMathFunctionLibrary::CompareInts(
      static_cast<int>(knowledge->GetDetectionState()),
      static_cast<int>(instanceData.RequiredDetectionState),
      instanceData.ComparisonMethod);
}

bool FTATStateTreeConditionTargetWithAttitude::TestCondition(FStateTreeExecutionContext& context) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if(instanceData.Target == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeConditionTargetWithAttitude failed since Target is missing."));
      return false;
   }
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeConditionTargetWithAttitude failed since AIController is missing."));
      return false;
   }

   const ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if (aiController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeConditionTargetWithAttitude failed since AIController is not a TATAIController"));
      return false;
   }

   const FTATActorKnowledge* knowledge = aiController->GetTATKnowledgeComponent()->GetActorKnowledge(instanceData.Target);
   if(knowledge == nullptr)
   {
      return false;
   }
   return UOSEMathFunctionLibrary::CompareInts(
      static_cast<int>(knowledge->GetAttitude()),
      static_cast<int>(instanceData.RequiredTeamAttitude),
      instanceData.ComparisonMethod);
}
