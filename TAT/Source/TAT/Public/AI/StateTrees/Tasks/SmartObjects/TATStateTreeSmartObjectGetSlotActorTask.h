// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "SmartObjectTypes.h"
#include "SmartObjectSubsystem.h"
#include "StateTreeTaskBase.h"

#include "TATStateTreeSmartObjectGetSlotActorTask.generated.h"

// This is almost a carbon copy of Engine/Plugins/Runtime/GameplayInteractions/Source/GameplayInteractionsModule/Private/StateTree/GameplayInteractionGetSlotActorTask.h
// Because we want to make our own modifications without also modifying the engine version I decided to duplicate it, which has an added benefit of
// removing the dependency on the GameplayInteractions Module.

USTRUCT()
struct FTATStateTreeSmartObjectGetSlotActorTaskInstanceData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category="Input")
   FSmartObjectSlotHandle TargetSlot;
   
   UPROPERTY(EditAnywhere, Category="Output")
	TObjectPtr<AActor> ResultActor;
};

USTRUCT(meta = (DisplayName = "[TAT] Get Slot Actor", Category="TAT|Smart Object"))
struct FTATStateTreeSmartObjectGetSlotActorTask : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()

   FTATStateTreeSmartObjectGetSlotActorTask();
   using FInstanceDataType = FTATStateTreeSmartObjectGetSlotActorTaskInstanceData;

protected:
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

   virtual bool Link(FStateTreeLinker& linker) override;
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;

   /** If true, and no valid actor is found, the task will fail. */
   UPROPERTY(EditAnywhere, Category = "Parameter")
   bool _bFailIfNotFound = true;
   
   TStateTreeExternalDataHandle<USmartObjectSubsystem> _SmartObjectSubsystemHandle;
};
