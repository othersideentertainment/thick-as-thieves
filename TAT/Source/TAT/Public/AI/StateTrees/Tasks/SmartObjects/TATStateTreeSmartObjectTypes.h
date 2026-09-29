// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "TATStateTreeSmartObjectFindSlotTask.h"
#include "UObject/Object.h"

#include "TATStateTreeSmartObjectTypes.generated.h"

// This is almost a carbon copy of Engine/Plugins/Runtime/GameplayInteractions/Source/GameplayInteractionsModule/Public/GameplayInteractionsTypes.h
// Because we want to make our own modifications without also modifying the engine version I decided to duplicate it, which has an added benefit of
// removing the dependency on the GameplayInteractions Module.

UENUM()
enum class ETATStateTreeSmartObjectTaskTrigger : uint8
{
   /** Execute when state becomes active. */
   OnEnterState,
	
   /** Execute when state becomes inactive. */
   OnExitState,

   /** Execute if the state fails. */
   OnExitStateFailed,

   /** Execute if the state succeeds. */
   OnExitStateSucceeded,
};

USTRUCT(BlueprintType)
struct TAT_API FTATSmartObjectInteractionSlotUserData : public FSmartObjectSlotStateData
{
   GENERATED_BODY()

   FTATSmartObjectInteractionSlotUserData() = default;
   explicit FTATSmartObjectInteractionSlotUserData(AActor* inUserActor) : UserActor(inUserActor) {}
		
   TWeakObjectPtr<AActor> UserActor = nullptr;
};
