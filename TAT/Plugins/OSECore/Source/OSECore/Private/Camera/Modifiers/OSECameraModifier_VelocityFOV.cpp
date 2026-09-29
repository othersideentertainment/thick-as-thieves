// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Camera/Modifiers/OSECameraModifier_VelocityFOV.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSECameraModifier_VelocityFOV)


UOSECameraModifier_VelocityFOV::UOSECameraModifier_VelocityFOV()
{
   Priority = 64;
}

// Allows modifying the camera in native code.
void UOSECameraModifier_VelocityFOV::ModifyCamera(float DeltaTime, FVector ViewLocation, FRotator ViewRotation, float FOV, FVector& NewViewLocation, FRotator& NewViewRotation, float& NewFOV)
{
   // Copy the values initially
   NewViewLocation = ViewLocation;
   NewViewRotation = ViewRotation;
   NewFOV = FOV;

   // This updates the field of view boost based on the settings.
   // This includes an optional interpolation speed, which is why we feed
   // the results back in to the same variable.
   CurrentFOVBoost = UOSECameraUtils::UpdateFieldOfViewBoost(
      FOVBoostSettings,
      GetVelocityForward(ViewRotation),
      DeltaTime,
      CurrentFOVBoost);

   // Update the new field of view with the boost
   NewFOV += CurrentFOVBoost;

   // Assume the default distance from the focal plane
   const FVector CameraForwardVec = ViewRotation.RotateVector(FVector::ForwardVector);
   float DistanceToFocalPlane = 0.0f;

   // Check for a player controller so we can adjust the plane distance based on our focal position
   APlayerController* PlayerController = (CameraOwner != nullptr) ? CameraOwner->GetOwningPlayerController() : nullptr;
   if (PlayerController != nullptr)
   {
      const FVector ToFocalLocation = PlayerController->GetFocalLocation() - NewViewLocation;
      DistanceToFocalPlane = ToFocalLocation | CameraForwardVec;
   }

   // Compute the offset and smooth it out as well
   const float NewOffset = UOSECameraUtils::GetFieldOfViewDollyOffset(FOV, NewFOV, DistanceToFocalPlane);
   CurrentOffsetDist = FMath::FInterpTo(CurrentOffsetDist, NewOffset, DeltaTime, FOVBoostSettings.InterpolationSpeed);

   // Update the offset vector with the distance
   const FVector OffsetVector = CameraForwardVec * CurrentOffsetDist;
   NewViewLocation += OffsetVector;
}

// Resets any state dependent on the velocity
void UOSECameraModifier_VelocityFOV::ResetVelocity()
{
   Super::ResetVelocity();
   CurrentFOVBoost = 0.0f;
   CurrentOffsetDist = 0.0f;
}

