// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Interactables/InteractableInterface.h"
#include "Items/ItemActorRuntimeStateOwnerInterface.h"

// ue4
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "ItemActor.generated.h"

class AItemActorRuntimeState;

UCLASS(Abstract)
class OSEITEM_API AItemActor 
   : public AActor
   , public IInteractableInterface
   , public IItemActorRuntimeStateOwnerInterface
{
   GENERATED_BODY()
   
public:

   AItemActor();

public:
   // from AActor
   virtual void BeginPlay() override;

   // from IInteractableInterface
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt) override;

   UFUNCTION(BlueprintPure, Category = "Items")
   AItemActorRuntimeState* GetRuntimeStateActor() const { return _runtimeStateActor; }

   UFUNCTION(BlueprintPure, Category = "Items")
   bool GetWasDropped() const { return _wasDropped; }
   
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Items")
   void AuthoritySpawnRuntimeStateActor();

   virtual AItemActorRuntimeState* AuthorityGetOrCreateRuntimeStateActor();
   virtual void AuthoritySetRuntimeStateActor(AItemActorRuntimeState* runtimeStateActor);

   // from IOSEItemActorRuntimeStateOwnerInterface
   virtual void AuthorityAddItemActorRuntimeState(TSubclassOf<AActor> itemActorClass, AItemActorRuntimeState* runtimeState) override;
   virtual void AuthorityClearItemActorRuntimeState(TSubclassOf<AActor> itemActorClass) override;
   virtual AItemActorRuntimeState* AuthorityGetItemActorRuntimeState(TSubclassOf<AActor> itemActorClass) override;

   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   void HandleDrop(const FVector& dropOrigin, const FVector& intendedDropDestination, APawn* droppingPawn);

protected:

   // Called when taken by the local player.
   UFUNCTION(BlueprintImplementableEvent, Category = "Items")
   void OnTakenLocally();

   // Called on Server when taken.
   UFUNCTION(BlueprintImplementableEvent, Category = "Items")
   void OnTakenAuthority(ACharacter* inTakingCharacter);

   // Called when dropped by the local player.
   UFUNCTION(BlueprintImplementableEvent, Category = "Items")
   void OnDroppedLocally();

   // Called on Server when Dropped.
   UFUNCTION(BlueprintImplementableEvent, Category = "Items")
   void OnDroppedAuthority(const FVector& dropOrigin, const FVector& intendedDropDestination, APawn* droppingPawn);

   virtual void _OnTakenAuthority(ACharacter* inTakingCharacter) {}
   virtual void _OnDroppedAuthority(const FVector& dropOrigin, const FVector& intendedDropDestination, APawn* droppingPawn) {}

protected:
   UPROPERTY(EditDefaultsOnly, Category = "Item")
   TSubclassOf<AItemActorRuntimeState> RuntimeStateClass;

private:
   UPROPERTY(Transient, Replicated)
   AItemActorRuntimeState* _runtimeStateActor = nullptr;

   UPROPERTY(Transient, ReplicatedUsing=_OnRep_WasDropped)
   bool _wasDropped = false;

   UFUNCTION()
   void _OnRep_WasDropped();
};
