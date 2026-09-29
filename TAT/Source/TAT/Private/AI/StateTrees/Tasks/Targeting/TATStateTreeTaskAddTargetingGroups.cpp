// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/StateTrees/Tasks/Targeting/TATStateTreeTaskAddTargetingGroups.h"

// tat
#include "AI/TATAIController.h"

// ue
#include "AIController.h"
#include "Engine/AssetManager.h"
#include "StateTreeExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskAddTargetingGroups)

FTATStateTreeTaskAddTargetingGroups::FTATStateTreeTaskAddTargetingGroups()
{
   // No tick needed.
   bShouldCallTick = false;

   // Prevent re-entering when an active child state transitions.
   bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FTATStateTreeTaskAddTargetingGroups::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);

   if (instanceData.Assets.IsEmpty())
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskAddTargetingGroups::EnterState called with no targeting considerations to add."));
      return EStateTreeRunStatus::Failed;
   }

   const ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskAddTargetingGroups::EnterState failed since AIController is not a TATAIController."));
      return EStateTreeRunStatus::Failed;
   }

   UTATStateTreeTargetingComponent* targetingComponent = aiController->GetStateTreeTargetingComponent();
   if (targetingComponent == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskAddTargetingGroups::EnterState failed to retrieve UTATStateTreeTargetingComponent."));
      return EStateTreeRunStatus::Failed;
   }

   TArray<FSoftObjectPath> softObjectPaths;
   softObjectPaths.Reserve(instanceData.Assets.Num());
   for (const auto& asset : instanceData.Assets)
   {
      softObjectPaths.Add(asset.ToSoftObjectPath());
   }
   instanceData.AssetsLoadingHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(softObjectPaths),
      [weakTargetingComponent = MakeWeakObjectPtr(targetingComponent), assets = instanceData.Assets]
   {
      UTATStateTreeTargetingComponent* targetingComponent = weakTargetingComponent.Get();
      if (targetingComponent == nullptr)
      {
         return;
      }

      for (const TSoftObjectPtr<UTATStateTreeTargetingConsiderations>& asset : assets)
      {
         targetingComponent->AddTargetingConsiderations(asset.Get());
      }
   });

   return EStateTreeRunStatus::Running;
}

void FTATStateTreeTaskAddTargetingGroups::ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   FInstanceDataType& instanceData = context.GetInstanceData(*this);

   if (instanceData.Assets.IsEmpty())
   {
      return;
   }

   const ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
   if (instanceData.AIController == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskAddTargetingGroups::ExitState failed since AIController is not a TATAIController"));
      return;
   }

   UTATStateTreeTargetingComponent* targetingComponent = aiController->GetStateTreeTargetingComponent();
   if (targetingComponent == nullptr)
   {
      UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskAddTargetingGroups::ExitState failed to retrieve UTATStateTreeTargetingComponent."));
      return;
   }

   if (instanceData.AssetsLoadingHandle->HasLoadCompleted())
   {
      for (TSoftObjectPtr<UTATStateTreeTargetingConsiderations>& asset : instanceData.Assets)
      {
         targetingComponent->RemoveTargetingConsiderations(asset.Get());
      }
   }
   else
   {
      // If the asset loading didn't finish, just cancel it.
      // Given it didn't finish, none of the assets would have been applied.
      instanceData.AssetsLoadingHandle->CancelHandle();
   }
}
