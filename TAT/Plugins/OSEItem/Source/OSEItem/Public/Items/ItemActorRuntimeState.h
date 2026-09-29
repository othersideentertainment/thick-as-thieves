// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "ItemInfo.h"

// ue4
#include "GameFramework/Actor.h"

#include "ItemActorRuntimeState.generated.h"

class APlayerState;

UCLASS(Abstract)
class OSEITEM_API AItemActorRuntimeState : public AActor
{
   GENERATED_BODY()

public:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRuntimeStateChanged);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemInfoClassChanged, TSubclassOf<UItemInfo>, itemInfoClass);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOwningActorChanged, AActor*, owningActor);

public:
   AItemActorRuntimeState();

   // from AActor
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
   virtual bool IsNetRelevantFor(const AActor* realViewer, const AActor* viewTarget, const FVector& srcLocation) const override;

   // item info for this runtime state
   void AuthoritySetItemInfoClass(TSubclassOf<UItemInfo> itemInfoClass);
   UFUNCTION(BlueprintPure, Category = "Items")
   TSubclassOf<UItemInfo> GetItemInfoClass() const { return _itemInfoClass; }
   UPROPERTY(BlueprintAssignable, Category = "Items")
   FOnItemInfoClassChanged OnItemInfoClassChanged;

   // set when an object takes ownership over this runtime state
   void AuthoritySetOwningActor(AActor* owningActor);
   UFUNCTION(BlueprintPure, Category = "Items")
   AActor* GetOwningActor() const { return _owningActor; }
   UPROPERTY(BlueprintAssignable, Category = "Items")
   FOnOwningActorChanged OnOwningActorChanged;

   // generic "something about my runtime state changed" event so the UI can just fully refresh itself.
   // callable by blueprints when it detects a change it wants the UI to fetch
   UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Items")
   FOnRuntimeStateChanged OnRuntimeStateChanged;

private:
   UFUNCTION()
   void OnRep_ItemInfoClass();
   void _BroadcastItemInfoClassChanged();

   UFUNCTION()
   void OnRep_OwningActor();
   void _BroadcastOwningActorChanged();

private:
   UPROPERTY(ReplicatedUsing = OnRep_ItemInfoClass)
   TSubclassOf<UItemInfo> _itemInfoClass;
   UPROPERTY(ReplicatedUsing = OnRep_OwningActor)
   AActor* _owningActor = nullptr;
};
