// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "SmartObjectTypes.h"
#include "SmartObjectSubsystem.h"
#include "StateTreeTaskBase.h"

#include "TATStateTreeSmartObjectFindSlotTask.generated.h"

// This is almost a carbon copy of Engine/Plugins/Runtime/GameplayInteractions/Source/GameplayInteractionsModule/Private/StateTree/GameplayInteractionFindSlotTask.h
// Because we want to make our own modifications without also modifying the engine version I decided to duplicate it, which has an added benefit of
// removing the dependency on the GameplayInteractions Module.

USTRUCT()
struct FTATStateTreeSmartObjectFindSlotTaskInstanceData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category="Input")
   FSmartObjectSlotHandle ReferenceSlot;
   UPROPERTY(EditAnywhere, Category="Output")
   FSmartObjectSlotHandle ResultSlot;
};

USTRUCT(meta = (DisplayName = "[TAT] Find Slot", Category="TAT|Smart Object"))
struct FTATStateTreeSmartObjectFindSlotTask : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()

   FTATStateTreeSmartObjectFindSlotTask();
   using FInstanceDataType = FTATStateTreeSmartObjectFindSlotTaskInstanceData;

protected:
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

   virtual bool Link(FStateTreeLinker& linker) override;
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;

   UPROPERTY(EditAnywhere, Category="Parameter")
   FGameplayTag _FindByTag;
   
   TStateTreeExternalDataHandle<USmartObjectSubsystem> _SmartObjectSubsystemHandle;
};
