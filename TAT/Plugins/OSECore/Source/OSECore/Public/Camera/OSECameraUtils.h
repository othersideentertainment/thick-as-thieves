// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OSECameraUtils.generated.h"


//--------------------------------------------------------------------------------------------------
/// Names for camera components, if you want to use a different class
/// (with ObjectInitializer.SetDefaultSubobjectClass)
//--------------------------------------------------------------------------------------------------

struct OSECORE_API FCameraComponentName
{
   static constexpr auto ParentXfm = TEXT("CameraParentXfm");
   static constexpr auto StartXfm = TEXT("CameraStartXfm");
   static constexpr auto SpringArm = TEXT("CameraSpringArm");
   static constexpr auto EndXfm = TEXT("CameraEndXfm");
   static constexpr auto Component = TEXT("CameraComponent");
};


//--------------------------------------------------------------------------------------------------
/// Velocity based field of view boost settings
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FOSEFieldOfViewBoost
{
   GENERATED_BODY()

public:

   /// Enable or disable the field of view boost
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera)
   bool Enabled = true;

   /// Maximum field of view change
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (editcondition = "Enabled", ClampMin = -45, UIMin = -45, ClampMax = 45, UIMax = 45))
   float Amount = 5;

   /// No change at this speed or lower
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (editcondition = "Enabled"))
   float VelocityMin = 200;

   /// Full change at this speed or higher
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (editcondition = "Enabled"))
   float VelocityMax = 700;

   /// Ramping curve for the min and max velocity. Raises the velocity T value to this power
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (editcondition = "Enabled", ClampMin = 0.1, UIMin = 0.1, ClampMax = 16, UIMax = 16))
   float VelocityRamp = 1.5f;

   /// Interpolation speed for field of view change. Low values are slower (more lag), high values are faster (less lag), while zero is instant (no lag)
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (editcondition = "Enabled", ClampMin = 0, UIMin = 0, ClampMax = 1000, UIMax = 1000))
   float InterpolationSpeed = 10;
};


//--------------------------------------------------------------------------------------------------
/// Camera utilities
//--------------------------------------------------------------------------------------------------

UCLASS()
class OSECORE_API UOSECameraUtils : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:

   /// Updates and returns the current field of view boost given the camera component.
   /// Velocity is derived from the camera owner's velocity in the camera direction.
   UFUNCTION(BlueprintPure, Category = "Camera|OSE")
   static float UpdateFieldOfViewBoostForCamera(
      const FOSEFieldOfViewBoost& FOVBoostSettings,
      const class UCameraComponent* CameraComponent,
      const float DeltaTime,
      const float CurrentFOVBoost);

   /// Updates and returns the current field of view boost given the velocity.
   UFUNCTION(BlueprintPure, Category = "Camera|OSE")
   static float UpdateFieldOfViewBoost(
      const FOSEFieldOfViewBoost& FOVBoostSettings,
      const float CameraVelocity,
      const float DeltaTime,
      const float CurrentFOVBoost);

   /// Returns the desired field of view boost given the velocity (no interpolation taken into account).
   UFUNCTION(BlueprintPure, Category = "Camera|OSE")
   static float GetFieldOfViewBoost(
      const FOSEFieldOfViewBoost& FOVBoostSettings,
      const float CameraVelocity);

   /// Returns the offset required to dolly zoom for the given focal plane, based on a change to field of view.
   /// The field of view is specified in degrees.
   /// The focal plane distance and the returned offset is assumed to be local to the camera plane defined by
   /// the view (forward) vector.
   UFUNCTION(BlueprintPure, Category = "Camera|OSE")
   static float GetFieldOfViewDollyOffset(
      const float OriginalFOV,
      const float NewFOV,
      const float FocalPlaneDistance);

   /// Returns a view projection matrix (sometimes refered to as a "world to frustum" matrix).
   /// 
   /// \note This is NOT suitable for rendering, as it is not reverse-z. However, it is suitable for game purposes
   /// when a frustum or camera needs to do some culling or frustum work.
   /// 
   /// \note The field of view is a half angle, specified in degrees.
   /// 
   /// \note DrawDebugFrustum expects a "frustum to world" matrix, so use the Inverse() of the returned value for that call.
   /// 
   UFUNCTION(BlueprintPure, Category = "Camera|OSE")
   static FMatrix BuildViewProjectionMatrix(
      const FVector& viewOrigin, const FRotator& viewRotation,
      float fieldOfView, float aspectRatio,
      float nearPlane, float farPlane);
};
