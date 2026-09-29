// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Math/UnrealMathUtility.h"
#include "PlayerCameraAnim.generated.h"


//--------------------------------------------------------------------------------------------------
/// Structure defining a socket and relative transform
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FPlayerCameraSocket
{
   GENERATED_BODY()

public:

   FPlayerCameraSocket() { }
   FPlayerCameraSocket(
      const FName SocketName,
      const FRotator& RelRotation = FRotator::ZeroRotator,
      const FVector& RelLocation = FVector::ZeroVector);

   FORCEINLINE FTransform GetRelativeTransform() const { return FTransform(Rotation, Location); }

   /// Socket on the player mesh to use for camera positioning / orientation
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera)
   FName Name = NAME_None;

   /// Additional offset to apply to the socket
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera)
   FVector Location = FVector::ZeroVector;

   /// Additional rotation to apply to the socket
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera)
   FRotator Rotation = FRotator::ZeroRotator;
};


//--------------------------------------------------------------------------------------------------
/// Structure defining camera animation settings
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FPlayerCameraAnimation
{
   GENERATED_BODY()

public:

   FPlayerCameraAnimation() { }
   FPlayerCameraAnimation(float InPosWeight, float InRotWeight, float InDirOverride, float InCenterAimOverride);

   FORCEINLINE bool HasWeight() const { return ((PositionWeight > 0.0f) || (RotationWeight > 0.0f)); }

   /// The weight from 0..1 of the animation used to drive camera position
   UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (ClampMin = 0, UIMin = 0, ClampMax = 1, UIMax = 1))
   float PositionWeight = 0.075f;

   /// The weight from 0..1 of the animation used to drive camera rotation
   UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (ClampMin = 0, UIMin = 0, ClampMax = 1, UIMax = 1))
   float RotationWeight = 0.075f;

   /// The amount from 0..1 to override the rotation of the animated camera.
   /// A value of 0 is no override, while a value of 1 will replace the rotation with a new
   /// orthogonal basis pointing forward. Certain animations may look better with this setting.
   UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (ClampMin = 0, UIMin = 0, ClampMax = 1, UIMax = 1))
   float ForwardDirectionOverride = 0.0f;

   /// The amount from 0..1 to re-center the local player's aim during the animation. This works by simulating local player
   /// camera inputs (for example, it simulates using a PS5 controller right thumbstick to adjust the player's camera view).
   ///
   /// A value of 0 is no override, while a value of 1 will attempt to re-center at full speed. The speed of the re-centering
   /// is based on this setting and is limited by the maximum turn / look rate that is specified when using an analog stick.
   UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (ClampMin = 0, UIMin = 0, ClampMax = 1, UIMax = 1))
   float CenterAimOverride = 0.0f;
};

template<>
FORCEINLINE_DEBUGGABLE FPlayerCameraAnimation FMath::Lerp(const FPlayerCameraAnimation& A, const FPlayerCameraAnimation& B, const float& Alpha)
{
   FPlayerCameraAnimation Result;
   Result.PositionWeight = FMath::Lerp(A.PositionWeight, B.PositionWeight, Alpha);
   Result.RotationWeight = FMath::Lerp(A.RotationWeight, B.RotationWeight, Alpha);
   Result.ForwardDirectionOverride = FMath::Lerp(A.ForwardDirectionOverride, B.ForwardDirectionOverride, Alpha);
   Result.CenterAimOverride = FMath::Lerp(A.CenterAimOverride, B.CenterAimOverride, Alpha);
   return Result;
}
/*
template<>
FORCEINLINE_DEBUGGABLE FPlayerCameraAnimation FMath::Max(const FPlayerCameraAnimation A, const FPlayerCameraAnimation B)
{
   FPlayerCameraAnimation Result;
   Result.PositionWeight = FMath::Max(A.PositionWeight, B.PositionWeight);
   Result.RotationWeight = FMath::Max(A.RotationWeight, B.RotationWeight);
   Result.ForwardDirectionOverride = FMath::Max(A.ForwardDirectionOverride, B.ForwardDirectionOverride);
   Result.CenterAimOverride = FMath::Max(A.CenterAimOverride, B.CenterAimOverride);
   return Result;
}
*/
