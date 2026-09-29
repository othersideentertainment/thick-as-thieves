// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE4
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

// OSE
#include "Animation/Graph/OSEAnimData.h"
#include "OSEAnimDataLibrary.generated.h"


//--------------------------------------------------------------------------------------------------
/// Function library utilities to compute animation data
//--------------------------------------------------------------------------------------------------

UCLASS(Const, meta = (BlueprintThreadSafe))
class OSECORE_API UOSEAnimDataFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:

   /// Returns the state value given a pawn
   // 
   // @TODO: Hiding BP exposed method until API solidifies
   //        UFUNCTION(BlueprintPure, Category = "AnimationData|OSE", meta = (NotBlueprintThreadSafe))
   static EOSEAnimState GetAnimState(const class APawn* pawnOwner);
   static EOSEAnimState GetAnimState(const class UPawnMovementComponent* pawnMovementComponent);
   static EOSEAnimState GetAnimState(EMovementMode movementMode, uint8 customMode);

   /// If the pawn is a character on a valid walkable floor, this returns true and the
   /// rotation matrix corresponding to the floor normal and actor forward vector.
   /// Otherwise, the rotation is set to the identity matrix and false is returned.
   //
   // @TODO: Hiding BP exposed method until API solidifies
   //        UFUNCTION(BlueprintPure, Category = "AnimationData|OSE", meta = (NotBlueprintThreadSafe))
   static bool GetFloorRotation(FRotator& outRotation, const class APawn* pawnOwner);
   static bool GetFloorRotation(FRotator& outRotation, const struct FFindFloorResult& floorResult, const FTransform& localToWorld);
   static FRotator GetFloorRotation(const FHitResult& hitResult, const FTransform& localToWorld);
   static TEnumAsByte<EPhysicalSurface> GetPhysicalSurfaceFromWall(const FFindWallResult& wallResult);
};

