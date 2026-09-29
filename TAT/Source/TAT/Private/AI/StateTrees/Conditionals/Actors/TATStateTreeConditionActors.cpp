// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Conditionals/Actor/TATStateTreeConditionActors.h"

// ue
#include "StateTreeExecutionContext.h"

// tat
#include "AI/TATAIController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeConditionActors)

bool FTATStateTreeConditionActor_HasGameplayTag::TestCondition(FStateTreeExecutionContext& context) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.Target == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeConditionActor_HasGameplayTag failed since Target is missing."));
      return false;
   }
   const IGameplayTagAssetInterface* assetInterface;
   if(const AAIController* aiController = Cast<AAIController>(instanceData.Target))
   {
      assetInterface =  Cast<IGameplayTagAssetInterface>(aiController->GetPawn()); 
   }
   else
   {
      assetInterface = Cast<IGameplayTagAssetInterface>(instanceData.Target);
   }
   if(assetInterface == nullptr)
   {
      return false;
   }

   const bool returnValue =  assetInterface->HasMatchingGameplayTag(instanceData.RequiredGameplayTag);
   if(instanceData.Invert)
   {
      return !returnValue; 
   }
   return returnValue;
}
