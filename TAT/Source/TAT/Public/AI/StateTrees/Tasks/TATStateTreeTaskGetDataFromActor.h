// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue 
#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"

#include "TATStateTreeTaskGetDataFromActor.generated.h"

class AAIController;

USTRUCT()
struct FTATStateTreeTaskActorLocationData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController = nullptr;

   UPROPERTY(EditAnywhere, Category = "Out", meta = (RefType = "/Script/CoreUObject.Vector"))
   FStateTreePropertyRef OutLocation;
};

USTRUCT(meta = (DisplayName = "Get Location From Actor", Category = "TAT|Data|Extractions"))
struct TAT_API FTATStateTreeTaskGetLocationFromActor : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskActorLocationData;
	
   FTATStateTreeTaskGetLocationFromActor() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};

USTRUCT()
struct FTATStateTreeTaskActorRotationData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController = nullptr;

   UPROPERTY(EditAnywhere, Category = "Out", meta = (RefType = "/Script/CoreUObject.Rotator"))
   FStateTreePropertyRef OutRotation;
};

USTRUCT(meta = (DisplayName = "Get Rotation From Actor", Category = "TAT|Data|Extractions"))
struct TAT_API FTATStateTreeTaskGetRotationFromActor : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskActorRotationData;

   FTATStateTreeTaskGetRotationFromActor() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};
