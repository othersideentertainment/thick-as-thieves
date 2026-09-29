// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "OSEMantleSettings.h"
#include "OSEMantleLocation.generated.h"


/// The mantle query result
UENUM(BlueprintType)
enum class EOSEMantleResult : uint8
{
   /// Invalid mantle location (the character could not be positioned here)
   Invalid,

   /// Valid mantle location (the character could be positioned here)
   /// This does not necessarily mean the character can actually reach the position
   /// or perform a mantle, however.
   Valid,
};


/// Mantle query result data
USTRUCT(BlueprintType)
struct OSECORE_API FOSEMantleQueryResult
{
   GENERATED_BODY()

   /// Returns true if the mantle location data is valid
   bool IsValid() const { return Result == EOSEMantleResult::Valid; }

   /// Whether or not the location data is valid
   UPROPERTY()
   EOSEMantleResult Result = EOSEMantleResult::Invalid;

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

   /// Final position hit result. This corresponds to where the capsule ends after a mantle.
   UPROPERTY()
   FHitResult FinalHitResult = FHitResult(EForceInit::ForceInit);
};


/// Minimal set of mantle state resolved from query result.
/// Suitable for network replication and movement mode snapshots.
USTRUCT(BlueprintType)
struct OSECORE_API FOSEMantleState
{
   GENERATED_BODY()

   UPROPERTY()
   bool IsValid = false;

   UPROPERTY()
   bool IsMantling = false;

   UPROPERTY()
   bool IsCrouching = false;

   UPROPERTY()
   FVector_NetQuantize CapsuleExtents = FVector::ZeroVector;

   UPROPERTY()
   TWeakObjectPtr<class UAnimMontage> Montage = nullptr;

   UPROPERTY()
   FVector_NetQuantize QueryLocation = FVector::ZeroVector;

   UPROPERTY()
   FVector_NetQuantizeNormal QueryDirection = FVector::ForwardVector;

   UPROPERTY()
   FVector_NetQuantize StartLocation = FVector::ZeroVector;

   UPROPERTY()
   FVector_NetQuantizeNormal StartDirection = FVector::ForwardVector;

   UPROPERTY()
   TWeakObjectPtr<class UPhysicalMaterial> StartPhysicalMaterial = nullptr;
   
   UPROPERTY()
   FVector_NetQuantize FinalLocation = FVector::ZeroVector;

   UPROPERTY()
   FVector_NetQuantizeNormal FinalDirection = FVector::ForwardVector;

   FORCEINLINE bool operator==(const FOSEMantleState& other) const
   {
      return
         IsValid == other.IsValid &&
         IsMantling == other.IsMantling &&
         CapsuleExtents == other.CapsuleExtents &&
         Montage == other.Montage &&
         QueryLocation == other.QueryLocation &&
         QueryDirection == other.QueryDirection &&
         StartLocation == other.StartLocation &&
         StartDirection == other.StartDirection &&
         FinalLocation == other.FinalLocation &&
         FinalDirection == other.FinalDirection;
   }

   FORCEINLINE bool operator!=(const FOSEMantleState& other) const
   {
      return
         IsValid != other.IsValid ||
         IsMantling != other.IsMantling ||
         CapsuleExtents != other.CapsuleExtents ||
         Montage != other.Montage ||
         QueryLocation != other.QueryLocation ||
         QueryDirection != other.QueryDirection ||
         StartLocation != other.StartLocation ||
         StartDirection != other.StartDirection ||
         FinalLocation != other.FinalLocation ||
         FinalDirection != other.FinalDirection;
   }
};

