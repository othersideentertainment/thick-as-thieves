// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

#include "TATClientProxyActorTransform.generated.h"

/// A world transform that has an optional rotation and scale, and a custom NetSerialize that only sends those values if used.
/// See also: UTATThiefVisionSubsystem
USTRUCT()
struct FTATClientProxyActorTransform
{
   GENERATED_BODY()

   UPROPERTY()
   FVector_NetQuantize Location = FVector::ZeroVector;

   UPROPERTY()
   bool AllowRotation = false;

   UPROPERTY()
   FRotator Rotation = FRotator::ZeroRotator;

   UPROPERTY()
   bool AllowScale = false;

   UPROPERTY()
   FVector_NetQuantize Scale = FVector::OneVector;

   FTATClientProxyActorTransform() = default;

   FTATClientProxyActorTransform(const FTransform& transform, bool allowRotation, bool allowScale)
   {
      Set(transform, allowRotation, allowScale);
   }

   FORCEINLINE FTransform Get() const
   {
      return FTransform(
         AllowRotation ? Rotation : FRotator::ZeroRotator,
         Location,
         AllowScale ? Scale : FVector::OneVector);
   }

   void Set(const FTransform& transform, bool allowRotation, bool allowScale);

   FORCEINLINE bool operator==(const FTATClientProxyActorTransform& rhs) const
   {
      return Location == rhs.Location
         && AllowRotation == rhs.AllowRotation
         && AllowScale == rhs.AllowScale
         && (!AllowRotation || Rotation == rhs.Rotation)
         && (!AllowScale || Scale == rhs.Scale);
   }

   FORCEINLINE bool operator!=(const FTATClientProxyActorTransform& rhs) const { return !operator==(rhs); }

   bool NetSerialize(FArchive& ar, UPackageMap* packageMap, bool& outSuccess);
};

template<>
struct TStructOpsTypeTraits<FTATClientProxyActorTransform> : public TStructOpsTypeTraitsBase2<FTATClientProxyActorTransform>
{
   enum
   {
      WithNetSerializer = true,
   };
};
