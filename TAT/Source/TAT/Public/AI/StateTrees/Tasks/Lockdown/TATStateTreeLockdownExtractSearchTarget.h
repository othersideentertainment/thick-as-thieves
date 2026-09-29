// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "AIController.h"
#include "StateTreeTaskBase.h"

// tat
#include "AI/Environment/TATAIContextualLocation.h"
#include "AI/Squad/TATSquadAlarmStation.h"

#include "TATStateTreeLockdownExtractSearchTarget.generated.h"

USTRUCT()
struct FTATStateTreeLockdownExtractSearchTargetData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController { nullptr };
   
   UPROPERTY(EditAnywhere, Category = Out)
   ATATAIContextualLocation* LocationToSearch { nullptr };
   UPROPERTY(EditAnywhere, Category = Out)
   FVector LocationVectorToSearch { ForceInit };
};

USTRUCT()
struct TAT_API FTATStateTreeLockdownExtractSearchTarget : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeLockdownExtractSearchTargetData;
	
   FTATStateTreeLockdownExtractSearchTarget() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   virtual void ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};

USTRUCT()
struct FTATStateTreeLockdownData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController { nullptr };
};

USTRUCT()
struct TAT_API FTATStateTreeLockdown : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeLockdownData;
	
   FTATStateTreeLockdown();
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   virtual void ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};

USTRUCT()
struct FTATStateTreeLockdownGetAlarmStationData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController { nullptr };
   
   UPROPERTY(EditAnywhere, Category = Out)
   ATATSquadAlarmStation* AlarmStation { nullptr };
};

USTRUCT()
struct TAT_API FTATStateTreeLockdownGetAlarmStation : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeLockdownGetAlarmStationData;
	
   FTATStateTreeLockdownGetAlarmStation() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};


USTRUCT()
struct FTATStateTreeLockdownFinishSearchingData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController { nullptr };
};

USTRUCT()
struct TAT_API FTATStateTreeLockdownFinishSearching : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeLockdownFinishSearchingData;
	
   FTATStateTreeLockdownFinishSearching() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};
