// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "SmartObjectTypes.h"
#include "SmartObjectSubsystem.h"
#include "StateTreeTaskBase.h"

#include "TATStateTreeSmartObjectListenSlotEventsTask.generated.h"

// This is almost a carbon copy of Engine/Plugins/Runtime/GameplayInteractions/Source/GameplayInteractionsModule/Private/StateTree/GameplayInteractionListenSlotEventsTask.h
// Because we want to make our own modifications without also modifying the engine version I decided to duplicate it, which has an added benefit of
// removing the dependency on the GameplayInteractions Module.

USTRUCT()
struct FTATStateTreeSmartObjectListenSlotEventsTaskInstanceData
{
   GENERATED_BODY()
   
   UPROPERTY(EditAnywhere, Category="Input")
   FSmartObjectSlotHandle ReferenceSlot;
   FDelegateHandle OnEventHandle;
};

USTRUCT(meta = (DisplayName = "[TAT] Listen Slot Events", Category="TAT|Smart Object"))
struct FTATStateTreeSmartObjectListenSlotEventsTask  : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()

   FTATStateTreeSmartObjectListenSlotEventsTask();
   using FInstanceDataType = FTATStateTreeSmartObjectListenSlotEventsTaskInstanceData;

protected:
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

   virtual bool Link(FStateTreeLinker& linker) override;
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   virtual void ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;

   TStateTreeExternalDataHandle<USmartObjectSubsystem> _SmartObjectSubsystemHandle;
};
