// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "SmartObjectRuntime.h"
#include "SmartObjectSubsystem.h"
#include "StateTreeExecutionContext.h"
#include "UObject/Object.h"

#include "TATSmartObjectStateTreeContext.generated.h"

struct FGameplayInteractionAbortContext;
class AAIController;
class UTATSmartObjectBehaviorDefinition;

USTRUCT()
struct FTATSmartObjectStateTreeContext
{
   GENERATED_BODY()
   
public:
	const FSmartObjectClaimHandle& GetClaimedHandle() const { return _ClaimedHandle; }
	void SetClaimedHandle(const FSmartObjectClaimHandle& inClaimedHandle) { _ClaimedHandle = inClaimedHandle; }

	void SetSlotEntranceHandle(const FSmartObjectSlotEntranceHandle& inSlotEntranceHandle) { _SlotEntranceHandle = inSlotEntranceHandle; }
		
	void SetContextPawn(APawn* InContextPawn) { _ContextPawn = InContextPawn; }
	void SetContextController(AAIController* inContextController) { _ContextController = inContextController; }
	void SetSmartObjectActor(AActor* InSmartObjectActor) { _SmartObjectActor = InSmartObjectActor; }

	bool IsValid() const { return _ClaimedHandle.IsValid() && _ContextPawn != nullptr && _ContextController != nullptr && _SmartObjectActor != nullptr; }

	bool Activate(const UTATSmartObjectBehaviorDefinition& definition);
	bool Tick(const float deltaTime);
	void Deactivate();

	void SendEvent(const FGameplayTag tag, const FConstStructView payload = FConstStructView(), const FName origin = FName());

protected:	
	bool SetContextRequirements(FStateTreeExecutionContext& stateTreeContext);
	bool ValidateSchema(const FStateTreeExecutionContext& stateTreeContext) const;
	
	UPROPERTY()
	FStateTreeInstanceData _StateTreeInstanceData;
    
   UPROPERTY()
   FSmartObjectClaimHandle _ClaimedHandle;

	UPROPERTY()
	FSmartObjectSlotEntranceHandle _SlotEntranceHandle;

   UPROPERTY()
   TObjectPtr<APawn> _ContextPawn { nullptr };
   
   UPROPERTY()
   TObjectPtr<AAIController> _ContextController { nullptr };
    
   UPROPERTY()
   TObjectPtr<AActor> _SmartObjectActor { nullptr };

	UPROPERTY()
	TObjectPtr<const UTATSmartObjectBehaviorDefinition> _Definition { nullptr };
};
