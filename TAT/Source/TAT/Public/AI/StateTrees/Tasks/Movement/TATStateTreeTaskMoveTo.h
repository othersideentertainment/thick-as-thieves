// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeMoveToTask.h"

#include "TATStateTreeTaskMoveTo.generated.h"

USTRUCT()
struct FTATStateTreeMoveToTaskInstanceData : public FStateTreeMoveToTaskInstanceData
{
   GENERATED_BODY()
   
   UPROPERTY(EditAnywhere, Category = Parameter)
   bool bContinuousGoalTracking = true;
   UPROPERTY(EditAnywhere, Category = Parameter)
   bool bShouldDirectMove = false;
};

USTRUCT(meta = (DisplayName = "[TAT] Move To", Category = "TAT|AI|Action"))
struct FTATStateTreeMoveToTask : public FStateTreeMoveToTask
{
   GENERATED_BODY()

   using FInstanceDataType = FTATStateTreeMoveToTaskInstanceData;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual UAITask_MoveTo* PrepareMoveToTask(FStateTreeExecutionContext& context, AAIController& controller, UAITask_MoveTo* existingTask, FAIMoveRequest& moveRequest) const override;
};
