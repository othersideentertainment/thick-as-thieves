// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"
#include "UObject/Object.h"

#include "TATStateTreeTaskUpdateKnowledgeOfActor.generated.h"


class AAIController;

USTRUCT()
struct FTATStateTreeTaskUpdateKnowledgeOfActorData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController = nullptr;
   
   UPROPERTY(EditAnywhere, Category=In)
   AActor* Target = nullptr;
   
   UPROPERTY(EditAnywhere, meta = (RefType = "/Script/CoreUObject.Vector"))
   FStateTreePropertyRef ResultLastKnownLocation;
};

USTRUCT()
struct TAT_API FTATStateTreeTaskUpdateKnowledgeOfActor : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskUpdateKnowledgeOfActorData;
	
   FTATStateTreeTaskUpdateKnowledgeOfActor() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   UPROPERTY(EditAnywhere)
   bool ShouldTrackLocation { false };
   
   EStateTreeRunStatus SetLastKnownLocation(FStateTreeExecutionContext& context, bool failIfNotSet) const;
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& context, const float deltaTime) const override;
};
