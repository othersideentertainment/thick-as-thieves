// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OSEMantleSettings.h"
#include "OSEMantleLocation.h"
#include "OSEMantleQuery.generated.h"


/// Mantle utility methods
UCLASS()
class OSECORE_API UOSEMantleQuery : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:

   /// Traces for a mantle location given a capsule component and the specified velocity
   /// for look-ahead projection.
   UFUNCTION(BlueprintPure, Category = Traversal)
   static FOSEMantleQueryResult TraceMantleCapsuleWithVelocity(
      const FOSEMantleSettings& MantleSettings,
      const class UCapsuleComponent* Capsule,
      const FRotator& SearchRotation,
      const FVector& Velocity);

   /// Traces for a mantle location given a capsule component, a starting position, and the
   /// specified velocity for look-ahead projection.
   UFUNCTION(BlueprintPure, Category = Traversal)
   static FOSEMantleQueryResult TraceMantleCapsuleWithVelocityAndPosition(
         const FOSEMantleSettings& MantleSettings,
         const class UCapsuleComponent* Capsule,
         const FVector& StartingPosition,
         const FRotator& SearchRotation,
         const FVector& Velocity);

   /// Traces for a mantle location given a character and the specified velocity
   /// for look-ahead projection.
   UFUNCTION(BlueprintPure, Category = Traversal)
   static FOSEMantleQueryResult TraceMantleCharacterWithVelocity(
      const FOSEMantleSettings& MantleSettings,
      const class ACharacter* Character,
      const FVector& Velocity);

   /// Traces for a mantle location given a character. The velocity of the character
   /// is used for look-ahead projection.
   UFUNCTION(BlueprintPure, Category = Traversal)
   static FOSEMantleQueryResult TraceMantleCharacter(
      const FOSEMantleSettings& MantleSettings,
      const class ACharacter* Character);

private:

   static void GatherAllValidMantleLocations(
      TArray< FOSEMantleQueryResult >& ValidMantleList,
      const FOSEMantleSettings& MantleSettings,
      const FOSEMantleQueryResult& LocationData,
      const class UCapsuleComponent* Capsule,
      const FVector& ProjectedMin,
      const FVector& ProjectedMax);

   /// Utility method to get sweep params
   static const UWorld* GetMantleSweepParams(
      struct FCollisionQueryParams& QueryParams,
      struct FCollisionResponseParams& ResponseParams,
      enum ECollisionChannel& CollisionChannel,
      const class UCapsuleComponent* Capsule);

   /// Utility method for constructing a scaled capsule trace shape, with height/radius scaled by parameters
   static FCollisionShape MakeCapsuleTraceShape(const class UCapsuleComponent* capsule, const float heightScale = 1.0f, const float radiusScale = 1.0f);

   /// Utility method for single capsule sweep
   static bool SweepMantleSingle(
      FHitResult& HitResult,
      const class UCapsuleComponent* Capsule,
      const FVector& TraceStart,
      const FVector& TraceEnd,
      const float HeightScale = 1.0f,
      const float RadiusScale = 1.0f);

   /// Utility method for multi capsule sweep
   static bool SweepMantleMulti(
      TArray<FHitResult>& OutHits,
      const class UCapsuleComponent* Capsule,
      const FVector& TraceStart,
      const FVector& TraceEnd,
      const float HeightScale = 1.0f,
      const float RadiusScale = 1.0f);

   /// Utility method for a sphere sweep
   static bool SweepSphereSingle(
      FHitResult& hitResult,
      const class UCapsuleComponent* Capsule,
      const FVector& traceStart,
      const FVector& traceEnd,
      const float sphereRadius);

   /// Utility method to validate final hit results. Uses similar logic to UCharacterMovementComponent::CanStepUp
   /// without the movement mode check, since we want to support mantle while falling.
   static bool IsValidMantleResult(
      const FHitResult& hitResult,
      const class UCapsuleComponent* capsule);

   /// Filters hit results using to ensure only valid final hit results are considered
   static bool FilterValidMantleResults(
      TArray<FHitResult>& outHits,
      const class UCapsuleComponent* capsule);
};
