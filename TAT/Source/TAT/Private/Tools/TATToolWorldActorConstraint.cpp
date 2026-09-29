// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/TATToolWorldActorConstraint.h"

// ue5
#include "Engine/HitResult.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

// tat
#include "TATCollision.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATToolWorldActorConstraint)

// static
bool UTATToolWorldActorConstraint::CanPlaceWorldActor(TSubclassOf<UTATToolWorldActorConstraint> constraintClass, AActor* playerPawn, const FHitResult& hitResult)
{
   if (UTATToolWorldActorConstraint* constraint = constraintClass ? constraintClass->GetDefaultObject<UTATToolWorldActorConstraint>() : nullptr)
   {
      return constraint->_CanPlaceWorldActor(playerPawn, hitResult);
   }
   else
   {
	  // No constraint means we can place anywhere
      return true;
   }
}

bool UTATToolWorldActorConstraint::_CanPlaceWorldActor(AActor* playerPawn, const FHitResult& hitResult) const
{
   // Should be overriden by classes
   checkNoEntry();
   return false;
}

bool UTATToolWorldActorConstraint_EnoughSpaceToTeleport::_CanPlaceWorldActor(AActor* playerPawn, const FHitResult& hitResult) const
{
   // Require that we actually hit something
   if (!hitResult.bBlockingHit)
   {
      return false;
   }

   if (const ACharacter* playerCharacter = Cast<const ACharacter>(playerPawn))
   {
      // Start the teleport query from our position, offset by half the character's height
      // Otherwise, the query assumes we're trying to teleport into the ground and can get confused
      FTATTeleportTargetParams teleportParams = {};
      teleportParams.MarkerLocation = hitResult.Location + FVector(0.0f, 0.0f, playerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
      teleportParams.MarkerFacing = playerCharacter->GetActorForwardVector();

      // We don't need to know the location, just that one is possible from this placement
      FVector unusedLocation;
      return UTATTeleportUtilities::CalculateTeleportLocation(playerCharacter, teleportParams, TeleportSettings, unusedLocation);
   }
   else
   {
      // Not a character, so just ignore
      return false;
   }
}

bool UTATToolWorldActorConstraint_PlacedOnSurface::_CanPlaceWorldActor(AActor* _playerPawn, const FHitResult& hitResult) const
{
   // Require that we actually hit something
   return hitResult.bBlockingHit;
}

bool UTATToolWorldActorConstraint_PlacedOnSurfaceAvoidToolWorldActor::_CanPlaceWorldActor(AActor* playerPawn, const FHitResult& hitResult) const
{
   // Require that we actually hit something
   if (!hitResult.bBlockingHit)
   {
      return false;
   }

   if (UWorld* world = playerPawn->GetWorld())
   {
      // If there are any other Tool World Actors within range, don't allow the tool to be placed here.
      if (world->OverlapAnyTestByObjectType(hitResult.ImpactPoint, FQuat::Identity, COLLISION_TOOL_WORLD_ACTOR, FCollisionShape::MakeSphere(MinDistanceToOtherActors)))
      {
         return false;
      }
   }

   return true;
}

bool UTATToolWorldActorConstraint_BlueprintBase::_CanPlaceWorldActor(AActor* playerPawn, const FHitResult& hitResult) const
{
   return BP_CanPlaceWorldActor(playerPawn, hitResult);
}
