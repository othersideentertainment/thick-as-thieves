// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TATTeleportUtilities.generated.h"


class ACharacter;

struct TAT_API FTATTeleportTargetParams
{
   FVector MarkerLocation;
   FVector MarkerFacing;
};

/// Settings to configure the teleport query
/// Will likely be tuned in relation to the radius of the projectile and the character capsule size,
/// but are left as absolute values rather than coefficients, as the nature of the relationships is
/// as yet unclear.
USTRUCT()
struct TAT_API FTATTeleportQuerySettings
{
   GENERATED_BODY()

public:
   /// Radius of sphere to use when checking for "floor"s (really any obstacle below)
   UPROPERTY(EditDefaultsOnly, meta = (Units = cm))
   float FloorCheckRadius = 15.f;

   /// Radius of sphere to use when checking above the obstacle.
   /// This is a separate value from the floor to allow it to better tolerate slightly wonky geo or mild overhang angles
   UPROPERTY(EditDefaultsOnly, meta = (Units = cm))
   float CeilingCheckRadius = 10.f;

   /// Extra vertical offset to add when adjusting for a floor
   UPROPERTY(EditDefaultsOnly, meta = (Units = cm))
   float VerticalOvershoot = 10.f;

   /// Distance to look back to try to sweep tatards the target from
   UPROPERTY(EditDefaultsOnly, meta = (Units = cm))
   float LookbackDistance = 80.f;
};

UCLASS()
class TAT_API UTATTeleportUtilities : public UObject
{
   GENERATED_BODY()


public:
   static bool CalculateTeleportLocation(const ACharacter* characterToTeleport, const FTATTeleportTargetParams& target, const FTATTeleportQuerySettings& settings, FVector& outLocation);
   static void VisualizeTeleportLocation(const ACharacter* characterToTeleport, const FTATTeleportTargetParams& target, const FTATTeleportQuerySettings& settings);


   UFUNCTION(BlueprintCallable, Category = Teleport)
   static bool FindTeleportAttachComponent(const AActor* teleportingActor, const FHitResult& hit, USceneComponent*& outAttachComponent);
   
};
