// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/ItemActorRuntimeState.h"

// ose
#include "Items/ItemActorRuntimeStateOwnerInterface.h"

// ue4
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemActorRuntimeState)

AItemActorRuntimeState::AItemActorRuntimeState()
   : Super()
{
   bReplicates = true;
   PrimaryActorTick.bCanEverTick = false;
   PrimaryActorTick.bStartWithTickEnabled = false;
}

void AItemActorRuntimeState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
   DOREPLIFETIME(AItemActorRuntimeState, _itemInfoClass);
   DOREPLIFETIME(AItemActorRuntimeState, _owningActor);
}

bool AItemActorRuntimeState::IsNetRelevantFor(const AActor* realViewer, const AActor* viewTarget, const FVector& srcLocation) const
{
   check(HasAuthority());

   bool isRelevant = Super::IsNetRelevantFor(realViewer, viewTarget, srcLocation);
   if (!isRelevant)
   {
      // allow our relevancy to match our owning actor, which should make us effectively always relevant when owned by player state, and less
      // relevant when owned by an item actor in the world
      isRelevant = _owningActor && _owningActor->IsNetRelevantFor(realViewer, viewTarget, srcLocation);
   }
   return isRelevant;
}

void AItemActorRuntimeState::AuthoritySetItemInfoClass(TSubclassOf<UItemInfo> itemInfoClass)
{
   check(HasAuthority());
   check(itemInfoClass);
   _itemInfoClass = itemInfoClass;
   _BroadcastItemInfoClassChanged();
}

void AItemActorRuntimeState::AuthoritySetOwningActor(AActor* owningActor)
{
   check(HasAuthority());
   _owningActor = owningActor;
   if (_owningActor)
   {
      // should implement the owner interface...
      check(_owningActor->Implements<UItemActorRuntimeStateOwnerInterface>());
   }
   _BroadcastOwningActorChanged();
}

void AItemActorRuntimeState::OnRep_ItemInfoClass()
{
   _BroadcastItemInfoClassChanged();
}

void AItemActorRuntimeState::_BroadcastItemInfoClassChanged()
{
   OnItemInfoClassChanged.Broadcast(_itemInfoClass);
}

void AItemActorRuntimeState::OnRep_OwningActor()
{
   _BroadcastOwningActorChanged();
}

void AItemActorRuntimeState::_BroadcastOwningActorChanged()
{
   OnOwningActorChanged.Broadcast(_owningActor);
}

