// (c) 2021-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE4
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

// OSE
#include "OSECameraSettings.generated.h"


//--------------------------------------------------------------------------------------------------
/// Camera parameters and settings for camera actors, camera components, etc.
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FOSECameraParams
{
   GENERATED_BODY()

public:
   
   /// if true, will smooth out the Z position while moving over rough terrain.
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PlayerCamera)
   bool ShouldSmoothWorldZPosition = false;

   /// The maximum distance that the camera will lag behind on the Z axis when calculating against the terrain.
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PlayerCamera)
   float MaxCameraZOffsetDistance = 50.0f;

   /// Interpolation speed specifically for the smoothing of the camera Z world position. Allowing us to differ from the crouching interpolation speed.
   UPROPERTY(Config, EditAnywhere, Category = CameraSmoothing, meta = (ClampMin = 0, UIMin = 0, ClampMax = 100, UIMax = 100))
   float WorldZInterpolationSpeed = 20.0f;

   /// The player's default horizontal field of view (in degrees). This value should match the camera
   /// settings used in DCC packages for best results (i.e. cinematic 1p animations in Maya)
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PlayerCamera, meta = (UIMin = "5.0", UIMax = "170", ClampMin = "0.001", ClampMax = "360.0", Units = deg))
   float FieldOfView = 75.0f;

   /// The player's default aspect ratio. This value should match the camera settings used in
   /// DCC packages for best results (i.e. cinematic 1p animations in Maya)
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PlayerCamera, meta = (ClampMin = "0.001", ClampMax = "100.0"))
   float AspectRatio = 1.777778f; // 16:9

   /// Minimum view pitch (in degrees). Used to limit the player's view.
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PlayerCamera, meta = (UIMin = "-89.9", UIMax = "-0.1", ClampMin = "-89.999", ClampMax = "-0.001", Units = deg))
   float ViewPitchMin = -75.0f;

   /// Maximum view pitch (in degrees). Used to limit the player's view.
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PlayerCamera, meta = (UIMin = "0.1", UIMax = "89.9", ClampMin = "0.001", ClampMax = "89.999", Units = deg))
   float ViewPitchMax = 85.0f;
};


//--------------------------------------------------------------------------------------------------
/// OSE camera settings.
//--------------------------------------------------------------------------------------------------

UCLASS(Config = Game, DefaultConfig, Const, Meta = (DisplayName = "[OSE] Camera Settings"))
class OSECORE_API UOSECameraSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:

   static const UOSECameraSettings& Get() { return *(GetDefault<UOSECameraSettings>()); }

   UFUNCTION(BlueprintPure, Category = "OSE|Camera")
   static const UOSECameraSettings* GetOSECameraSettings() { return GetDefault<UOSECameraSettings>(); }

   UFUNCTION(BlueprintGetter)
   const FOSECameraParams& GetPlayerParams() const { return _playerParams; }

   UFUNCTION(BlueprintGetter)
   float GetDampingInterpolationSpeed() const { return _dampingInterpolationSpeed; }

private:

   /// Default player camera parameters for camera components, camera manager, etc.
   UPROPERTY(Config, EditAnywhere, Category = CameraSettings, BlueprintGetter = GetPlayerParams, meta = (ConfigRestartRequired = true, ShowOnlyInnerProperties))
   FOSECameraParams _playerParams;

   /// Default damping rate for smoothing various transformations and blending.
   /// Low values are slower (more lag), high values are faster (less lag), while zero is instant (no lag).
   UPROPERTY(Config, EditAnywhere, Category = CameraSmoothing, BlueprintGetter = GetDampingInterpolationSpeed, meta = (ClampMin = 0, UIMin = 0, ClampMax = 100, UIMax = 100))
   float _dampingInterpolationSpeed = 10.0f;
};
