// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Camera/OSECameraUtils.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSECameraUtils)


// Updates and returns the current field of view boost given the camera component.
// Velocity is derived from the camera owner's velocity in the camera direction.
float UOSECameraUtils::UpdateFieldOfViewBoostForCamera(
   const FOSEFieldOfViewBoost& FOVBoostSettings,
   const UCameraComponent* CameraComponent,
   const float DeltaTime,
   const float CurrentFOVBoost)
{
   if (CameraComponent == nullptr || FOVBoostSettings.Enabled == false)
      return 0;

   // Velocity along the axis of the camera direction
   float CameraVelocity = CameraComponent->GetForwardVector() | CameraComponent->GetOwner()->GetVelocity();

   // Desired field of view delta
   return UpdateFieldOfViewBoost(FOVBoostSettings, CameraVelocity, DeltaTime, CurrentFOVBoost);
}

// Updates and returns the current field of view boost given the velocity.
float UOSECameraUtils::UpdateFieldOfViewBoost(
   const FOSEFieldOfViewBoost& FOVBoostSettings,
   const float CameraVelocity,
   const float DeltaTime,
   const float CurrentFOVBoost)
{
   if (FOVBoostSettings.Enabled == false)
      return 0;

   // Desired field of view delta
   const float DesiredDeltaFOV = GetFieldOfViewBoost(FOVBoostSettings, CameraVelocity);
   return FMath::FInterpTo(CurrentFOVBoost, DesiredDeltaFOV, DeltaTime, FOVBoostSettings.InterpolationSpeed);
}

// Returns the desired field of view boost given the velocity (no interpolation taken into account).
float UOSECameraUtils::GetFieldOfViewBoost(
   const FOSEFieldOfViewBoost& FOVBoostSettings,
   const float CameraVelocity)
{
   if (FOVBoostSettings.Enabled == false)
      return 0;

   // Velocity range percentage
   float CameraT = FMath::GetRangePct(FOVBoostSettings.VelocityMin, FOVBoostSettings.VelocityMax, CameraVelocity);
   CameraT = FMath::Clamp(CameraT, 0.0f, 1.0f);
   CameraT = FMath::Tan(FMath::DegreesToRadians(CameraT * 45.0f));
   CameraT = FMath::Pow(CameraT, FOVBoostSettings.VelocityRamp);

   // Desired field of view delta
   return FOVBoostSettings.Amount * CameraT;
}

// Returns the offset required to dolly zoom for the given focal plane, based on a change to field of view.
// The field of view is specified in degrees.
// The focal plane distance and the returned offset is assumed to be local to the camera plane defined by
// the view (forward) vector.
float UOSECameraUtils::GetFieldOfViewDollyOffset(
   const float OriginalFOV,
   const float NewFOV,
   const float FocalPlaneDistance)
{
   if (FocalPlaneDistance > 0.0f)
   {
      // Half of the field of view in radians
      const float HalfFOVOld = FMath::DegreesToRadians(OriginalFOV * 0.5f);
      const float HalfFOVNew = FMath::DegreesToRadians(NewFOV * 0.5f);

      // The new focal plane distance
      const float FocalDistNew = (FocalPlaneDistance * FMath::Tan(HalfFOVOld)) / FMath::Tan(HalfFOVNew);

      // We're interested in how the focal plane distance is different
      return (FocalPlaneDistance - FocalDistNew);
   }
   else
   {
      // It doesn't really make a lot of sense to have a focal plane behind the camera
      return 0;
   }
}

// IMPORTANT: See documentation / comments in the header!
FMatrix UOSECameraUtils::BuildViewProjectionMatrix(
   const FVector& viewOrigin, const FRotator& viewRotation,
   float fieldOfView, float aspectRatio,
   float nearPlane, float farPlane)
{
   const FMatrix viewRotationMatrix =
      FInverseRotationMatrix(viewRotation) * FMatrix(
         FPlane(0, 0, 1, 0),
         FPlane(1, 0, 0, 0),
         FPlane(0, 1, 0, 0),
         FPlane(0, 0, 0, 1));

   const FMatrix viewMatrix = FTranslationMatrix(-viewOrigin) * viewRotationMatrix;
   const FMatrix projMatrix = FPerspectiveMatrix(fieldOfView, aspectRatio, 1.0f, nearPlane, farPlane);
   return viewMatrix * projMatrix;
}

