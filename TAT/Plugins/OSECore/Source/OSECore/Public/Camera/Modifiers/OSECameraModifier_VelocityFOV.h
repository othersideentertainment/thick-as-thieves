// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Camera/Modifiers/OSECameraModifier.h"
#include "Camera/OSECameraUtils.h"
#include "OSECameraModifier_VelocityFOV.generated.h"


//--------------------------------------------------------------------------------------------------
/// Velocity field of view modifier. The settings that drive it can be set in class defaults,
/// from code, or from blueprints.
//--------------------------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable)
class OSECORE_API UOSECameraModifier_VelocityFOV : public UOSECameraModifier
{
   GENERATED_BODY()

public:

   UOSECameraModifier_VelocityFOV();

protected:

   /// Allows modifying the camera in native code.
   virtual void ModifyCamera(float DeltaTime, FVector ViewLocation, FRotator ViewRotation, float FOV, FVector& NewViewLocation, FRotator& NewViewRotation, float& NewFOV) override;

public:

   /// Returns the velocity field of view settings
   UFUNCTION(BlueprintGetter, Category = "CameraModifier|OSE")
   const FOSEFieldOfViewBoost& GetFOVBoostSettings() const { return FOVBoostSettings; }

   /// Sets the velocity field of view settings
   UFUNCTION(BlueprintSetter, Category = "CameraModifier|OSE")
   void SetFOVBoostSettings(const FOSEFieldOfViewBoost& InSettings) { FOVBoostSettings = InSettings; }

protected:

   /// Resets any state dependent on the velocity. Overridden here to also
   /// reset the current FOV boost, offset, etc.
   virtual void ResetVelocity() override;

protected:

   /// Parameters to control the velocity-based field of view boost
   UPROPERTY(EditAnywhere, BlueprintGetter = GetFOVBoostSettings, BlueprintSetter = SetFOVBoostSettings, Category = "CameraModifier", meta = (AllowPrivateAccess = "true"))
   FOSEFieldOfViewBoost FOVBoostSettings;

   UPROPERTY()
   float CurrentFOVBoost = 0.0f;

   UPROPERTY()
   float CurrentOffsetDist = 0.0f;
};
