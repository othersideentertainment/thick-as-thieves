// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/TargetActors/TATAbilityTargetActor_PlaceFront.h"

// ue4
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "WorldCollision.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAbilityTargetActor_PlaceFront)

void ATATAbilityTargetActor_PlaceFront::StartTargeting(UGameplayAbility* inAbility)
{
   Super::StartTargeting(inAbility);
   SourceActor = inAbility->GetCurrentActorInfo()->AvatarActor.Get();
}

bool ATATAbilityTargetActor_PlaceFront::ShouldProduceTargetData() const
{
   // Workaround for ShouldProduceTargetData being false on the server for AI, because it is erroneously only checking
   // MasterPC, which is explicitly a PlayerController. This is preferable to setting ShouldProduceTargetDataOnServer,
   // as that will have other, potentially undesirable side-effects on the server.
   // TODO: Consider engine mod?
   // NOTE: If you change this, also update the other ShouldProduceTargetData methods with the same fix.
   return Super::ShouldProduceTargetData() || (OwningAbility && OwningAbility->GetActorInfo().IsLocallyControlled());
}

void ATATAbilityTargetActor_PlaceFront::ConfirmTargetingAndContinue()
{
   check(ShouldProduceTargetData());
   if (SourceActor)
   {
      FGameplayAbilityTargetDataHandle handle = _PerformTargeting();
      TargetDataReadyDelegate.Broadcast(handle);
   }
}

FGameplayAbilityTargetDataHandle ATATAbilityTargetActor_PlaceFront::_PerformTargeting() const
{
   const UWorld* world = SourceActor->GetWorld();
   check(world);

   FVector unusedEyesPosition;
   FRotator eyeRotation;
   SourceActor->GetActorEyesViewPoint(unusedEyesPosition, eyeRotation);
   const FVector eyeDirection2D = eyeRotation.Vector().GetSafeNormal2D();
   
   const FVector startLocation = StartLocation.GetTargetingTransform().GetLocation();
   const FVector offsetStartLocation = startLocation + (eyeDirection2D * _forwardDistance);

   FCollisionQueryParams params(SCENE_QUERY_STAT(ATATAbilityTargetActor_PlaceFront), /*bTraceComplex*/ false);
   params.bReturnPhysicalMaterial = false;
   params.bIgnoreTouches = true;
   params.AddIgnoredActor(SourceActor);

   // Sweep a slightly inflated sphere downward to find any floor/ramp/obstavle
   FVector sweptLocation;
   {
      const float sweepRadius = _capsuleRadius + _downwardSweepPadding;
      const FVector sweepEnd = offsetStartLocation + FVector(0, 0, sweepRadius);
      const FVector sweepStart = offsetStartLocation + FVector(0, 0, sweepRadius + _downwardSweepStartOffset);
      FHitResult hit;
      world->SweepSingleByProfile(hit, sweepStart, sweepEnd, FQuat::Identity, _collisionProfile.Name, FCollisionShape::MakeSphere(sweepRadius), params);
      sweptLocation = hit.bBlockingHit ? hit.Location : sweepEnd;
   }

   // position the capsule with the bottom at the end of the sweep, and check if it has any blocking overlaps
   const FVector candidateCenter = sweptLocation + FVector(0, 0, _capsuleHalfHeight - _capsuleRadius); // explicitly not adjusting for padding
   bool isOverlapping = world->OverlapAnyTestByProfile(candidateCenter, FQuat::Identity, _collisionProfile.Name, FCollisionShape::MakeCapsule(_capsuleRadius, _capsuleHalfHeight), params);
   
   FGameplayAbilityTargetDataHandle result;

   if (!isOverlapping)
   {
      // bake location as the bottom of the capsule until we need something different (since that aligns with other assumptions)
      FGameplayAbilityTargetingLocationInfo endLocation;
      endLocation.LiteralTransform.SetLocation(candidateCenter - FVector(0, 0, _capsuleHalfHeight)); 
      
      result = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromLocations(StartLocation, endLocation);
   }

   return result;
}

