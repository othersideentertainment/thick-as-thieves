// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "AIController.h"
#include "StateTreeConditionBase.h"

// ose
#include "Math/OSEMathFunctionLibrary.h"
#include "TATStateTreeConditionActorDistanceFromOwner.generated.h"

USTRUCT()
struct FTATStateTreeConditionActorDistanceFromOwnerData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category="Context")
   AAIController* Controller { nullptr };

   UPROPERTY(EditAnywhere)
   AActor* Target { nullptr };
   
   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float ComparisonValue = 0.0f;

   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::LessThanOrEqualTo;
};

USTRUCT(meta = (DisplayName = "Check Distance From Owner", Category = "TAT|AI|Common"))
struct TAT_API FTATStateTreeConditionActorDistanceFromOwner : public FStateTreeConditionCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeConditionActorDistanceFromOwnerData;

   FTATStateTreeConditionActorDistanceFromOwner() = default;
   
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual bool TestCondition(FStateTreeExecutionContext& context) const override;
};
