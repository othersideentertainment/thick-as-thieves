// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "OSELightDetectionFunctionLibrary.generated.h"


UCLASS()
class OSELIGHTDETECTION_API UOSELightDetectionFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable)
   static float GetRangeMultiplierFromLightIntensity(const float& lightIntensity, UCurveFloat* lightIntensityToRangeMultiplierCurve); 
};
