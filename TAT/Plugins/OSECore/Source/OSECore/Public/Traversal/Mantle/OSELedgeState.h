// (c) OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

#include "OSELedgeState.generated.h"


USTRUCT(BlueprintType)
struct OSECORE_API FOSELedgeMountTarget
{
   GENERATED_BODY()

public:

   UPROPERTY()
   bool IsValid = false;

   UPROPERTY()
   bool AutoEject = false;

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
   FVector_NetQuantize AnchorLocation = FVector::ZeroVector;

   UPROPERTY()
   FVector_NetQuantizeNormal AnchorDirection = FVector::ForwardVector;

   UPROPERTY()
   FVector_NetQuantize MantleLocation = FVector::ZeroVector;

   UPROPERTY()
   FVector_NetQuantizeNormal MantleDirection = FVector::ForwardVector;

   FORCEINLINE bool operator==(const FOSELedgeMountTarget& other) const
   {
      return
         IsValid == other.IsValid &&
         CapsuleExtents == other.CapsuleExtents &&
         Montage == other.Montage &&
         QueryLocation == other.QueryLocation &&
         QueryDirection == other.QueryDirection &&
         StartLocation == other.StartLocation &&
         StartDirection == other.StartDirection &&
         MantleLocation == other.MantleLocation &&
         MantleDirection == other.MantleDirection;
   }

   FORCEINLINE bool operator!=(const FOSELedgeMountTarget& other) const
   {
      return
         IsValid != other.IsValid ||
         CapsuleExtents != other.CapsuleExtents ||
         Montage != other.Montage ||
         QueryLocation != other.QueryLocation ||
         QueryDirection != other.QueryDirection ||
         StartLocation != other.StartLocation ||
         StartDirection != other.StartDirection ||
         MantleLocation != other.MantleLocation ||
         MantleDirection != other.MantleDirection;
   }
};

/// Minimal set of ledge state resolved from query result.
/// Suitable for network replication and movement mode snapshots.
/// 
/// 
USTRUCT(BlueprintType)
struct OSECORE_API FOSELedgeState
{
   GENERATED_BODY()

public:

   UPROPERTY()
   bool IsLedgeStateActive = false;

   UPROPERTY()
   bool IsMounting = false;

   UPROPERTY()
   FOSELedgeMountTarget MountTarget;

   FORCEINLINE bool operator==(const FOSELedgeState& other) const
   {
      return
         IsLedgeStateActive == other.IsLedgeStateActive &&
         IsMounting == other.IsMounting &&
         MountTarget == other.MountTarget;
   }

   FORCEINLINE bool operator!=(const FOSELedgeState& other) const
   {
      return
         IsLedgeStateActive != other.IsLedgeStateActive ||
         IsMounting != other.IsMounting ||
         MountTarget == other.MountTarget;
   }
};
