// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/ItemActor.h"

// ose
#include "Items/ItemActorRuntimeState.h"

// ue4
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemActor)

#define LOCTEXT_NAMESPACE "ItemActor"

AItemActor::AItemActor()
{
   bReplicates = true;
   
   // start dormant until taken
   NetDormancy = DORM_Initial;
}

void AItemActor::BeginPlay()
{
   Super::BeginPlay();
}

void AItemActor::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt)
{
   outPrompt.PressAction = LOCTEXT("TakePrompt", "Take");
}

void AItemActor::AuthoritySpawnRuntimeStateActor()
{
   check(HasAuthority());
   ensure(AuthorityGetOrCreateRuntimeStateActor());
}

AItemActorRuntimeState* AItemActor::AuthorityGetOrCreateRuntimeStateActor()
{
   check(HasAuthority());
   if (RuntimeStateClass && !_runtimeStateActor)
   {
      _runtimeStateActor = GetWorld()->SpawnActor<AItemActorRuntimeState>(RuntimeStateClass);
   }
   return _runtimeStateActor;
}

void AItemActor::AuthoritySetRuntimeStateActor(AItemActorRuntimeState* runtimeStateActor)
{
   _runtimeStateActor = runtimeStateActor;
   check(_runtimeStateActor && _runtimeStateActor->GetClass() == RuntimeStateClass);
}

void AItemActor::AuthorityAddItemActorRuntimeState(TSubclassOf<AActor> itemActorClass, AItemActorRuntimeState* runtimeState)
{
   // we only want to own ourselves...
   check(itemActorClass == GetClass());
   _runtimeStateActor = runtimeState;

   // we only want to own our own runtime state class
   check(_runtimeStateActor && _runtimeStateActor->GetClass() == RuntimeStateClass);
}

void AItemActor::AuthorityClearItemActorRuntimeState(TSubclassOf<AActor> itemActorClass)
{
   // we only want to own ourselves...
   check(itemActorClass == GetClass());
   _runtimeStateActor = nullptr;
}

AItemActorRuntimeState* AItemActor::AuthorityGetItemActorRuntimeState(TSubclassOf<AActor> itemActorClass)
{
   if (itemActorClass == GetClass())
   {
      return _runtimeStateActor;
   }
   return nullptr;
}

void AItemActor::HandleDrop(const FVector& dropOrigin, const FVector& intendedDropDestination, APawn* droppingPawn)
{
   _wasDropped = true;

   if (HasAuthority())
   {
      //Call the internal virtual version.
      _OnDroppedAuthority(dropOrigin, intendedDropDestination, droppingPawn);
      //Call the BP Event.
      OnDroppedAuthority(dropOrigin, intendedDropDestination, droppingPawn);
   }

   OnDroppedLocally();
}

void AItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME_CONDITION(AItemActor, _wasDropped, COND_InitialOnly);
   DOREPLIFETIME(AItemActor, _runtimeStateActor);
}

void AItemActor::_OnRep_WasDropped()
{
   if (_wasDropped)
   {
      OnDroppedLocally();
   }
}

#undef LOCTEXT_NAMESPACE

