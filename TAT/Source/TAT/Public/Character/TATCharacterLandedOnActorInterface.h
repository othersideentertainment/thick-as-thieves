// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "UObject/Interface.h"

#include "TATCharacterLandedOnActorInterface.generated.h"

class ATATCharacterBase;

UINTERFACE(BlueprintType, MinimalAPI, Category = "TAT Character", meta = (CannotImplementInterfaceInBlueprint))
class UTATCharacterLandedOnActorInterface : public UInterface
{
   GENERATED_BODY()
};

/// Interface for actors who want to handle characters landing on them, optionally intercepting the landed event (which prevents side effects such as fall damage).
/// This can be added to actors that the character directly lands on via blocking collision, or actors that have overlap colliders (that also overlap with the ground).
///
/// Note that whenever multiple actors are landed on (eg. a puddle with an overlap box component and an actor under it with blocking collision),
/// the uppermost actor (the one that the character touched first) is the one that will receive the event.
class TAT_API ITATCharacterLandedOnActorInterface
{
   GENERATED_BODY()

public:
   /// Return true here to consider this actor when sorting actors to determine which one was "landed on".
   /// You should return false if the actor is in any kind of disabled state where it would not be possible to "land" on it.
   virtual bool WantsToHandleCharacterLandedEvents(ATATCharacterBase* character) const { return true; }

   /// Fired when a character lands on this actor.
   /// landedHit will always contain the original landed hit result.
   /// If this is fired from an overlapping collision shape, then overlappedComponent will contain that component (otherwise it will be null).
   /// If you set outInterceptLandedEvent to true, normal "character landed" behavior (such as fall damage) will be suppressed (this defaults to false).
   virtual void OnCharacterLandedOnThisActor(ATATCharacterBase* character, const FHitResult& landedHit, UPrimitiveComponent* overlappedComponent, bool& outInterceptLandedEvent) = 0;

};
