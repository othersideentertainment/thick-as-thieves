// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "AIResourceInterface.h"
#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"
#include "UObject/WeakInterfacePtr.h"

#include "TATStateTreeTaskWaitForAIResourceUnlocked.generated.h"

class AAIController;
class UActorComponent;

USTRUCT()
struct FTATStateTreeTaskWaitForAIResourceUnlockedData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController = nullptr;

   UPROPERTY(EditAnywhere, Category = "In", meta = (MustImplement = "/Script/AIModule.AIResourceInterface"))
   TSubclassOf<UActorComponent> AIResourceComponentClass;

   TWeakInterfacePtr<IAIResourceInterface> AIResourceInterface;
};

USTRUCT()
struct TAT_API FTATStateTreeTaskWaitForAIResourceUnlocked : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskWaitForAIResourceUnlockedData;
	
   FTATStateTreeTaskWaitForAIResourceUnlocked() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& context, const float deltaTime) const override;

private:
   EStateTreeRunStatus _GetState(const FStateTreeExecutionContext& context, const FString& contextString) const;
   const IAIResourceInterface* _GetOrFindResourceInterface(const FStateTreeExecutionContext& context, const FString& contextString) const;

};
