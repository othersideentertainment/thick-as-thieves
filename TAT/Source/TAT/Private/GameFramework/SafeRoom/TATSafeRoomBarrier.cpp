// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "GameFramework/SafeRoom/TATSafeRoomBarrier.h"

// tat
#include "GameFramework/SafeRoom/TATSafeRoom.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSafeRoomBarrier)

ATATSafeRoomBarrier::ATATSafeRoomBarrier()
{
   PrimaryActorTick.bCanEverTick = false;
}

void ATATSafeRoomBarrier::BeginPlay()
{
   Super::BeginPlay();

   if (ensure(IsValid(_safeRoom)))
   {
      _OnOwnerPawnChanged(_safeRoom->GetOwningPawn(), nullptr);
      _safeRoom->OnOwningPawnChanged.AddUObject(this, &ATATSafeRoomBarrier::_OnOwnerPawnChanged);

      _OnOwningPlayerChanged(_safeRoom->GetOwningPlayer());
      _safeRoom->OnOwningPlayerChanged.AddUniqueDynamic(this, &ATATSafeRoomBarrier::_OnOwningPlayerChanged);

      _OnOwnerTypeChanged(_safeRoom->GetOwnerType());
      _safeRoom->OnOwnerTypeChanged.AddUniqueDynamic(this, &ATATSafeRoomBarrier::_OnOwnerTypeChanged);
   }
}

void ATATSafeRoomBarrier::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (IsValid(_safeRoom))
   {
      _safeRoom->OnOwningPawnChanged.RemoveAll(this);
   }

   Super::EndPlay(endPlayReason);
}

void ATATSafeRoomBarrier::NotifyActorEndOverlap(AActor* otherActor)
{
   Super::NotifyActorEndOverlap(otherActor);

   // Cancels abilities with tag when the owning pawn passes through the barrier
   // Done on overlap exit, so that abilities started while overlapping the barrier do not have to be treated specially
   if(IsValid(_safeRoom) && (otherActor == _safeRoom->GetOwningPawn()) && _abilityTagToCancelOnTraversal.IsValid())
   {
      if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(otherActor, false))
      {
         FGameplayTagContainer tagsToCancel = _abilityTagToCancelOnTraversal.GetSingleTagContainer();
         asc->CancelAbilities(&tagsToCancel);
      }
   }
}

void ATATSafeRoomBarrier::_OnOwnerPawnChanged(APawn* newPawn, APawn* oldPawn)
{
   if (newPawn == oldPawn)
   {
      return;
   }

   APlayerState* newPS = newPawn ? newPawn->GetPlayerState() : nullptr;

   // If the owning player changes, un-ignore all of the old pawns before ignoring the new pawn
   // NB: We do this here instead of one of the other callbacks since the ordering is important
   if (newPS != _playerStateForIgnoredPawns.Get())
   {
      for (const TWeakObjectPtr<APawn>& ignoredPawn : _ignoredPawns)
      {
         _SetShouldIgnoreMovementWithPawn(ignoredPawn.Get(), false);
      }

      _ignoredPawns.Reset();
      _playerStateForIgnoredPawns = newPS;
   }

   _SetShouldIgnoreMovementWithPawn(newPawn, true);

   if (newPawn != nullptr)
   {
      _ignoredPawns.AddUnique(newPawn);
   }

   // TODO: Without respawn-in-place option, re-evaluate this
   // NOTE: for now, explicitly not removing IgnoreMovement with the previous pawn until the player control changes
   //       This prevents weirdness if the character is KOed in the middle of the
   //       barriers, and switches to astral projection.
}

void ATATSafeRoomBarrier::_SetShouldIgnoreMovementWithPawn(APawn* pawn, bool shouldIgnore)
{
   if (pawn == nullptr) return;

   // NOTE: This formerly was calling IgnoreActorWhenMoving. This was changed
   //       to component to allow the cancel-on-overlap to happen in the same
   //       actor, so that it is less error-prone to set up.
   //
   //       If we go down the road of having other targeting pass through this
   //       barrier as well for the owning player, we may want to switch it back.
   //       * Other than the only-slightly-supported `FMaskFilter IgnoreMask`,
   //         which only has six bits, ignoring actors and components are the only
   //         mechanism exposed to collision queries.
   //       * However, ignored actors have significantly more exposure in APIs, so
   //         something component-based would be much more intrusive, and possibly
   //         require re-writing a lot of boilerplate wrappers for things that do
   //         don't have it.
   //       * At that point, the workflow cost of having a separate actor would be
   //         outweighed by that of having to expose components.
   //       * Another possible alternative would be to still use the component here,
   //         but bubble up this actor to be ignored via a different mechanism.
   if (auto theirPrimitive = Cast<UPrimitiveComponent>(pawn->GetRootComponent()))
   {
      auto ourPrimitive = Cast<UPrimitiveComponent>(GetRootComponent());
      if (ensure(IsValid(ourPrimitive)))
      {
         theirPrimitive->IgnoreComponentWhenMoving(ourPrimitive, shouldIgnore);
      }
   }
}

void ATATSafeRoomBarrier::_OnOwningPlayerChanged(ATATPlayerState* owner)
{
   UPrimitiveComponent* ourPrimitive = Cast<UPrimitiveComponent>(GetRootComponent());
   if (ensure(IsValid(ourPrimitive)))
   {
      if (owner == nullptr)
      {
         ourPrimitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
      }
      else
      {
         ourPrimitive->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
      }
   }
}

void ATATSafeRoomBarrier::_OnOwnerTypeChanged(ETATSafeRoomOwnerType ownerType)
{
   BP_OnOwnerTypeChanged(ownerType);
}
