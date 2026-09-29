// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "SmartObjectTypes.h"
#include "StateTreeTaskBase.h"
#include "SmartObjectSubsystem.h"
#include "TATStateTreeSmartObjectTypes.h"

#include "TATStateTreeSmartObjectSendSlotEventTask.generated.h"

// This is almost a carbon copy of Engine/Plugins/Runtime/GameplayInteractions/Source/GameplayInteractionsModule/Private/StateTree/GameplayInteractionSendSlotEventTask.h
// Because we want to make our own modifications without also modifying the engine version I decided to duplicate it, which has an added benefit of
// removing the dependency on the GameplayInteractions Module.

USTRUCT()
struct FTATStateTreeSmartObjectSendSlotEventTaskInstanceData
{
   GENERATED_BODY()
   
   UPROPERTY(EditAnywhere, Category="Input")
   FSmartObjectSlotHandle ReferenceSlot;
   FDelegateHandle OnEventHandle;
};

USTRUCT(meta = (DisplayName = "[TAT] Send Slot Event", Category="TAT|Smart Object"))
struct FTATStateTreeSmartObjectSendSlotEventTask  : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()

   FTATStateTreeSmartObjectSendSlotEventTask();
   using FInstanceDataType = FTATStateTreeSmartObjectSendSlotEventTaskInstanceData;

protected:
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

   virtual bool Link(FStateTreeLinker& linker) override;
   
   virtual EDataValidationResult Compile(FStateTreeDataView instanceDataView, TArray<FText>& validationMessages) override;
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   virtual void ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;

   /** Tag of the event to send. */
   UPROPERTY(EditAnywhere, Category = Parameter)
   FGameplayTag _EventTag;

   /** Payload of the event to send. */
   UPROPERTY(EditAnywhere, Category = Parameter)
   FInstancedStruct _Payload;

   /** Specifies under which conditions to send the event. */
   UPROPERTY(EditAnywhere, Category = Parameter)
   ETATStateTreeSmartObjectTaskTrigger _Trigger { ETATStateTreeSmartObjectTaskTrigger::OnEnterState };
   
   /** If false, will not trigger on state reselection. */
   UPROPERTY(EditAnywhere, Category = Parameter)
   bool _bShouldTriggerOnReselect = true;
   
   /** If true, handle external State Tree stop as a failure. */
   UPROPERTY(EditAnywhere, Category = "Parameter")
   bool _bHandleExternalStopAsFailure = true;

   TStateTreeExternalDataHandle<USmartObjectSubsystem> _SmartObjectSubsystemHandle;
};
