// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OSELedgeSettings.h"
#include "OSEMantleLocation.h"
#include "OSELedgeQuery.generated.h"


/// Ledge query result data
USTRUCT(BlueprintType)
struct OSECORE_API FOSELedgeQueryResult
{
   GENERATED_BODY()

   /// Returns true if the mantle and ledge location data is valid
   bool IsValid() const { return MantleResult || LedgeResult; }

   /// Returns true if the ledge location data is valid
   bool IsValidLedge() const { return LedgeResult; }

   /// Returns true if the mantle location data is valid
   bool IsValidMantle() const { return MantleResult; }

   /// Whether or not the ledge location data is valid
   UPROPERTY()
   bool LedgeResult = false;

   /// Whether or not the mantle location data is valid
   UPROPERTY()
   bool MantleResult = false;

   /// Capsule radius
   UPROPERTY()
   float CapsuleRadius = 0;

   /// Capsule half height
   UPROPERTY()
   float CapsuleHalfHeight = 0;

   /// Capsule position at the start of the query
   UPROPERTY()
   FVector QueryLocation = FVector::ZeroVector;

   /// Capsule rotation at the start of the query
   UPROPERTY()
   FQuat QueryRotation = FQuat::Identity;

   /// Capsule velocity at the start of the query
   UPROPERTY()
   FVector QueryVelocity = FVector::ZeroVector;

   /// Start position hit result. This corresponds to where the capsule begins the mantle.
   UPROPERTY()
   FHitResult StartHitResult = FHitResult(EForceInit::ForceInit);

   /// Start position hit result. This corresponds to where the capsule begins the mantle.
   UPROPERTY()
   FHitResult LedgeHitResult = FHitResult(EForceInit::ForceInit);

   /// Final position hit result. This corresponds to where the capsule ends after a mantle.
   UPROPERTY()
   FHitResult FinalHitResult = FHitResult(EForceInit::ForceInit);
};


/// Mantle utility methods
UCLASS()
class OSECORE_API UOSELedgeQuery : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:


   /// Traces for a mantle location given a capsule component and the specified velocity
   /// for look-ahead projection.
   UFUNCTION(BlueprintPure, Category = Traversal)
   static FOSELedgeQueryResult TraceMantleCapsuleWithVelocity(
      const FOSELedgeSettings& ledgeSettings,
      const class UCapsuleComponent* Capsule,
      const FRotator& SearchRotation,
      const FVector& Velocity);

   /// Traces for a mantle location given a character and the specified velocity
   /// for look-ahead projection.
   UFUNCTION(BlueprintPure, Category = Traversal)
   static FOSELedgeQueryResult TraceMantleCharacterWithVelocity(
      const FOSELedgeSettings& ledgeSettings,
      const class ACharacter* Character,
      const FVector& Velocity);

   /// Traces for a mantle location given a character. The velocity of the character
   /// is used for look-ahead projection.
   UFUNCTION(BlueprintPure, Category = Traversal)
   static FOSELedgeQueryResult TraceMantleCharacter(
      const FOSELedgeSettings& ledgeSettings,
      const class ACharacter* Character);


   /// Traces for a mantle location given a character and the specified velocity
   /// for look-ahead projection.
   UFUNCTION(BlueprintPure, Category = Traversal)
   static FOSELedgeQueryResult TraceShimmyCharacterWithVelocity(
      const FOSELedgeSettings& ledgeSettings,
      const class ACharacter* Character,
      const FRotator& MoveRotation,
      const FVector& Velocity);

   static FOSELedgeQueryResult TraceShimmyCapsuleWithVelocity(
      const FOSELedgeSettings& ledgeSettings,
      const class UCapsuleComponent* Capsule,
      const FRotator& MoveRotation,
      const FRotator& SearchRotation,
      const FVector& Velocity);


   /// Traces for a mantle location given a character already on a ledge 
   static FOSELedgeQueryResult TraceFromLedgeCharacter(
         const FOSELedgeSettings& ledgeSettings,
         const class ACharacter* Character,
         const struct FOSELedgeState& CurrentLedge);

   static FOSELedgeQueryResult TraceFromLedgeCapsule(
      const FOSELedgeSettings& ledgeSettings,
      const class UCapsuleComponent* Capsule,
      const struct FOSELedgeState& CurrentLedge);

private:

   /// Traces for a mantle location given a capsule component and the specified velocity
   /// for look-ahead projection.
   static FOSELedgeQueryResult TraceMantle(
         const FOSELedgeSettings& ledgeSettings,
         FOSELedgeQueryResult& LocationData,
         const class UCapsuleComponent* Capsule,
         const FRotator& SearchRotation,
         const FVector& Velocity);

   static void GatherAllValidMantleLocations(
      TArray< struct FHitResult >& ValidMantleList,
      const FOSELedgeSettings& ledgeSettings,
      const FOSELedgeQueryResult& LocationData,
      const class UCapsuleComponent* Capsule,
      const FVector& ProjectedMin,
      const FVector& ProjectedMax,
      const float capsuleHalfHeight,
      const float capsuleRadius);

   /// Utility method to get sweep params
   static const UWorld* GetMantleSweepParams(
      struct FCollisionShape& TraceShape,
      struct FCollisionQueryParams& QueryParams,
      struct FCollisionResponseParams& ResponseParams,
      enum ECollisionChannel& CollisionChannel,
      const class UCapsuleComponent* Capsule,
      const float Height,
      const float Radius);

   /// Utility method for single capsule sweep
   static bool SweepMantleSingle(
      FHitResult& HitResult,
      const class UCapsuleComponent* Capsule,
      const FVector& TraceStart,
      const FVector& TraceEnd,
      const float Height,
      const float Radius);

   /// Utility method for single capsule sweep returning the first valid blocking hit
   static bool SweepMantleMulti(
      FHitResult& HitResult,
      const UCapsuleComponent* Capsule,
      const FVector& TraceStart,
      const FVector& TraceEnd,
      const float Height,
      const float Radius);

   /// Utility method to validate final hit results. Uses similar logic to UCharacterMovementComponent::CanStepUp
   /// without the movement mode check, since we want to support mantle while falling.
   static bool IsValidMantleResult(
      const FHitResult& hitResult,
      class APawn* pawnOwner);

   /// Filters hit results using to ensure only valid final hit results are considered
   static bool FilterValidMantleResults(
      TArray<FHitResult>& outHits,
      class APawn* pawnOwner);
};
