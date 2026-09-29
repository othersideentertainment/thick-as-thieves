// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose

// ue4
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"

#include "ItemActorRuntimeStateOwnerInterface.generated.h"

class AItemActorRuntimeState;

//---------------------------------------------------------------------------------------------------------
/// OSE ItemActor RuntimeStateOwner Interface
/// - Implement on objects that can be handed ownership of the ItemActor Runtime state (player 
///   state, pawn, and even item actors themselves.
//---------------------------------------------------------------------------------------------------------

UINTERFACE(BlueprintType, MinimalAPI, Category = "AI|OSE|Utility", meta = (CannotImplementInterfaceInBlueprint))
class UItemActorRuntimeStateOwnerInterface : public UInterface
{
   GENERATED_BODY()
};

class OSEITEM_API IItemActorRuntimeStateOwnerInterface
{
   GENERATED_BODY()

   // TODO: I want these properties to be TSubclassOf<AItemActor> but AItemActor wants to implement this interface, resulting in a cyclical dependency.

public:
   virtual void AuthorityAddItemActorRuntimeState(TSubclassOf<AActor> itemActorClass, AItemActorRuntimeState* runtimeState) = 0;
   virtual void AuthorityClearItemActorRuntimeState(TSubclassOf<AActor> itemActorClass) = 0;
   virtual AItemActorRuntimeState* AuthorityGetItemActorRuntimeState(TSubclassOf<AActor> itemActorClass) = 0;
};

UCLASS()
class OSEITEM_API UItemActorRuntimeStateOwnerFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Item Runtime State")
   static bool AuthorityTransferItemActorRuntimeState(TSubclassOf<AActor> itemActorClass, TScriptInterface<IItemActorRuntimeStateOwnerInterface> from, TScriptInterface<IItemActorRuntimeStateOwnerInterface> to);
};
