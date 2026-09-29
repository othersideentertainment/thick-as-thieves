// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Math/OSEMathFunctionLibrary.h"

// ue
#include "Engine/DeveloperSettings.h"

#include "OSELightDetectionSettings.generated.h"

USTRUCT()
struct OSELIGHTDETECTION_API FLightIntensityConfig
{
   GENERATED_BODY()
   
   /// Range to clamp directional light intensity within before resolving to a "detection" value
   UPROPERTY(EditDefaultsOnly)
   FFloatRange LuxRange = FFloatRange(0,1);
   
   /// Setting to opt into interpolated amplification of detection in directional light
   UPROPERTY(EditDefaultsOnly)
   bool RemapLightIntensityToRange = false;

   /// Mode used to interpolate the amplification added between 0% <-> 100%
   UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "RemapLightIntensityToRange"))
   EOSEInterpMode IntensityInterpMode = EOSEInterpMode::Linear;
   
   /// Range to remap directional light intensity to, before parsing as a detection value. 
   /// (i.e. if we want to increase overall detection in directional light, bring the lower bound up).
   UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "RemapLightIntensityToRange"))
   FFloatRange RemapRange = FFloatRange(0.0,1);
};

USTRUCT()
struct OSELIGHTDETECTION_API FLightIntensityClampingConfig
{
   GENERATED_BODY()
   UPROPERTY(EditDefaultsOnly)
   FFloatRange InputRange = FFloatRange(0,1);
   
   UPROPERTY(EditDefaultsOnly)
   float OutputValue { 0.f };
};
UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[OSE] Light Detection Settings"))
class OSELIGHTDETECTION_API UOSELightDetectionSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   static const UOSELightDetectionSettings& Get() { return *GetDefault<UOSELightDetectionSettings>(); }

   UPROPERTY(Config, EditDefaultsOnly)
   FLightIntensityConfig DirectionalLightSettings;

   UPROPERTY(Config, EditDefaultsOnly)
   bool ShouldClampLightBrightness { false };
   UPROPERTY(Config, EditDefaultsOnly, meta=(EditCondition="ShouldClampLightBrightness"))
   TArray<FLightIntensityClampingConfig> LightClampingConfig;
   
};
