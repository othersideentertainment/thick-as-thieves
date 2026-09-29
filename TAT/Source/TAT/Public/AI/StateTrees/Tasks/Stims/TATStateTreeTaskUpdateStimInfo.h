// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "StateTreePropertyRef.h"
#include "Tasks/StateTreeAITask.h"

#include "TATStateTreeTaskUpdateStimInfo.generated.h"

class AAIController;

USTRUCT()
struct FTATStateTreeTaskUpdateStimInfoData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController = nullptr;

   UPROPERTY(EditAnywhere, Category = "In")
   int StimID { INDEX_NONE };
   
   UPROPERTY(EditAnywhere, Category = "Out", meta = (RefType = "/Script/CoreUObject.Vector"))
   FStateTreePropertyRef OutStimInfoLocation;
};

USTRUCT(meta = (DisplayName = "Update Stim Info", Category = "TAT|AI|Stims"))
struct TAT_API FTATStateTreeTaskUpdateStimInfo : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskUpdateStimInfoData;
   FTATStateTreeTaskUpdateStimInfo() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& context, const float deltaTime) const override;

   EStateTreeRunStatus HandleUpdate(FStateTreeExecutionContext& context) const;
};
