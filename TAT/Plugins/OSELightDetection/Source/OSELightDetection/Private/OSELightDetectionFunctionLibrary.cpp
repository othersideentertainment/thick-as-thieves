// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "OSELightDetectionFunctionLibrary.h"

float UOSELightDetectionFunctionLibrary::GetRangeMultiplierFromLightIntensity(const float& lightIntensity,
   UCurveFloat* lightIntensityToRangeMultiplierCurve)
{
   if(lightIntensityToRangeMultiplierCurve == nullptr)
      return 1.f;
   return lightIntensityToRangeMultiplierCurve->GetFloatValue(lightIntensity);
}
