// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "AIController.h"
#include "StateTreeConditionBase.h"

// ose
#include "TATStateTreeConditionActorHasPathToOwner.generated.h"

USTRUCT()
struct FTATStateTreeConditionActorHasPathToOwnerData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category="Context")
   AAIController* Controller { nullptr };

   UPROPERTY(EditAnywhere)
   AActor* Target { nullptr };

   UPROPERTY(EditAnywhere)
   bool RequireAnyPath { true };
   
   UPROPERTY(EditAnywhere)
   bool RequireFullPath { true };
   
   UPROPERTY(EditAnywhere)
   bool RequireNoPath { false };
};

USTRUCT(meta = (DisplayName = "Has Path To Actor From Owner", Category = "TAT|AI|Common"))
struct TAT_API FTATStateTreeConditionActorHasPathToOwner : public FStateTreeConditionCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeConditionActorHasPathToOwnerData;

   FTATStateTreeConditionActorHasPathToOwner() = default;
   
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual bool TestCondition(FStateTreeExecutionContext& context) const override;
};
