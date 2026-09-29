// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/StateTrees/Targeting/TATStateTreeTargetingTypes.h"

// ue
#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"
#include "Templates/SharedPointer.h"

#include "TATStateTreeTaskAddTargetingGroups.generated.h"

class AAIController;
struct FStreamableHandle;

USTRUCT()
struct FTATStateTreeTaskAddTargetingGroupsData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController = nullptr;

   UPROPERTY(EditAnywhere, Category = "In")
   TSet<TSoftObjectPtr<UTATStateTreeTargetingConsiderations>> Assets;

   TSharedPtr<FStreamableHandle> AssetsLoadingHandle;
};

USTRUCT(meta = (DisplayName = "Track Targeting Groups", Category = "TAT|Targeting"))
struct TAT_API FTATStateTreeTaskAddTargetingGroups : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskAddTargetingGroupsData;

   FTATStateTreeTaskAddTargetingGroups();
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   virtual void ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;

};
