// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Traversal/TATTeleportUtilities.h"

// tat
#include "Developer/TATProjectSettings.h"

// ue4
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "GameplayTagAssetInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTeleportUtilities)


struct FTATTeleportContext
{
   static const ECollisionChannel kChannel = ECC_Pawn;

   FTATTeleportContext(const ACharacter* characterToTeleport, UWorld* world, const FTATTeleportTargetParams& target, const FTATTeleportQuerySettings& settings)
      : _character(characterToTeleport)
      , _world(world)
      , _target(target)
      , _settings(settings)
      , _queryParams(SCENE_QUERY_STAT(TATTeleport))
   {
      _queryParams.AddIgnoredActor(characterToTeleport);
      
      const UCapsuleComponent* capsule = _character->GetCapsuleComponent();
      capsule->GetScaledCapsuleSize(_capsuleRadius, _capsuleHalfHeight);
      capsule->InitSweepCollisionParams(_queryParams, _responseParams);

      // re-clobber just to be certain
      _queryParams.bReturnPhysicalMaterial = false;
      _queryParams.bTraceComplex = false;
   }


   FVector FindPossibleFloorAdjustment(const FVector& start) const
   {
      FHitResult hit;

      // check if there is something under a sphere
      {
         const FCollisionShape shape = FCollisionShape::MakeSphere(_settings.FloorCheckRadius);
         const FVector end = start + FVector(0, 0, -_capsuleHalfHeight);
         if (!_world->SweepSingleByChannel(hit, start, end, FQuat::Identity, kChannel, shape, _queryParams, _responseParams))
         {
            return FVector::ZeroVector;
         }
      }

      float possibleAdjustment = FMath::Max(_capsuleHalfHeight + _settings.VerticalOvershoot - (hit.Distance + _settings.FloorCheckRadius), 0.f);

      // check approximate clearance to adjust
      {
         const float headroomOffset = _capsuleHalfHeight - _settings.CeilingCheckRadius;
         const FVector end = start + FVector(0, 0, possibleAdjustment + headroomOffset);

         const FCollisionShape ceilingShape = FCollisionShape::MakeSphere(_settings.CeilingCheckRadius);
         if (_world->SweepSingleByChannel(hit, start, end, FQuat::Identity, kChannel, ceilingShape, _queryParams, _responseParams))
         {
            possibleAdjustment = FMath::Min(possibleAdjustment, hit.Distance - headroomOffset);
         }
      }

      return FVector(0, 0, possibleAdjustment);
   }

   // If there is a valid capsule space a configurable distance away in the travel distance from the target,
   // try sweeping the capsule tatard the target to find a valid location
   bool TryLookbackSweep(const FVector& targetPosition, FVector& outPosition) const
   {
      FVector offsetPosition = targetPosition - (_target.MarkerFacing * _settings.LookbackDistance);
      const FCollisionShape shape = FCollisionShape::MakeCapsule(_capsuleRadius, _capsuleHalfHeight);

      if (_world->OverlapBlockingTestByChannel(offsetPosition, FQuat::Identity, kChannel, shape, _queryParams, _responseParams))
      {
         // Try adjusting for floor. This can help if the marker is attached to a sloping overhang near the ground
         FVector floorAdjust = FindPossibleFloorAdjustment(offsetPosition);
         offsetPosition += floorAdjust;

         // if it still collides, bail
         if (floorAdjust.IsNearlyZero() || _world->OverlapBlockingTestByChannel(offsetPosition, FQuat::Identity, kChannel, shape, _queryParams, _responseParams))
         {
            return false;
         }
      }

      // sweep from valid location tatards target
      FHitResult hit;
      bool wasHit = _world->SweepSingleByChannel(hit, offsetPosition, targetPosition, FQuat::Identity, kChannel, shape, _queryParams, _responseParams);
      outPosition = wasHit ? hit.Location : targetPosition;
      check(FVector::DistSquared(outPosition, targetPosition) < FMath::Square(_settings.LookbackDistance + _capsuleHalfHeight));
      return true;
   }

private:
   const ACharacter* _character;
   UWorld* _world;
   const FTATTeleportTargetParams& _target;
   const FTATTeleportQuerySettings& _settings;
   float _capsuleHalfHeight;
   float _capsuleRadius;
   FCollisionQueryParams _queryParams;
   FCollisionResponseParams _responseParams;
};

bool UTATTeleportUtilities::CalculateTeleportLocation(const ACharacter* characterToTeleport, const FTATTeleportTargetParams& target, const FTATTeleportQuerySettings& settings, FVector& outLocation)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_UTATTeleportUtilities_CalculateTeleportLocation);
   if (characterToTeleport == nullptr) return false;

   UWorld* world = characterToTeleport->GetWorld();
   check(world);

   FTATTeleportContext context(characterToTeleport, world, target, settings);

   // Adjust for possible floor
   outLocation = target.MarkerLocation;
   outLocation += context.FindPossibleFloorAdjustment(target.MarkerLocation);

   // Try sweeping from the direction of travel
   FVector sweepLocation(ForceInit);
   if (context.TryLookbackSweep(outLocation, sweepLocation))
   {
      outLocation = sweepLocation;
      return true;
   }

   // Fall back to default position
   return world->FindTeleportSpot(characterToTeleport, outLocation, FRotator());
}

void UTATTeleportUtilities::VisualizeTeleportLocation(const ACharacter* characterToTeleport, const FTATTeleportTargetParams& target, const FTATTeleportQuerySettings& settings)
{
#if ENABLE_DRAW_DEBUG
   if (characterToTeleport == nullptr) return;

   UWorld* world = characterToTeleport->GetWorld();
   check(world);
   const UCapsuleComponent* capsule = characterToTeleport->GetCapsuleComponent();
   FVector teleportPoint = target.MarkerLocation;
   bool isValid = CalculateTeleportLocation(characterToTeleport, target, settings, teleportPoint);


   DrawDebugCapsule(world, teleportPoint, capsule->GetScaledCapsuleHalfHeight(), capsule->GetScaledCapsuleRadius(), FQuat::Identity, isValid ? FColor::Blue : FColor::Red);

   if (isValid && FVector::DistSquared(teleportPoint, target.MarkerLocation) > 10)
   {
      DrawDebugCapsule(world, target.MarkerLocation, capsule->GetScaledCapsuleHalfHeight(), capsule->GetScaledCapsuleRadius(), FQuat::Identity, FColor::Red);
   }
#endif
}

bool UTATTeleportUtilities::FindTeleportAttachComponent(const AActor* teleportingActor, const FHitResult& hit, USceneComponent*& outAttachComponent)
{
   outAttachComponent = nullptr;

   const AActor* const hitActor = hit.GetActor();

   // if no actor, that is fine
   if (!hitActor)
   {
      return true;
   }

   const UTATProjectSettings& settings = UTATProjectSettings::Get();
   UPrimitiveComponent* const hitComponent = hit.Component.Get();

   if (hitComponent)
   {
      for (FName blockedTag : settings.TeleportComponentBlockedTags)
      {
         if (hitComponent->ComponentHasTag(blockedTag))
         {
            return false;
         }
      }
   }

   if (const IGameplayTagAssetInterface* tagSource = Cast<IGameplayTagAssetInterface>(hitActor))
   {
      if (tagSource->HasAnyMatchingGameplayTags(settings.TeleportPawnBlockedTags))
      {
         return false;
      }
   }

   // if it is a pawn, always do the root component, regardless of what was hit
   const APawn* hitPawn = Cast<APawn>(hitActor);
   if (hitPawn && hitPawn->IsSupportedForNetworking())
   {
      outAttachComponent = hitPawn->GetRootComponent();
      return true;
   }

   if (hitComponent && 
      hitComponent->IsSupportedForNetworking() &&
      hitComponent->Mobility == EComponentMobility::Movable)
   {
      outAttachComponent = hitComponent;
      return true;
   }

   return true;
}

