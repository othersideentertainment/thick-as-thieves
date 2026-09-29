// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "UObject/Interface.h"

#include "OSELightDetectionInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class OSELIGHTDETECTION_API UOSELightDetectionInterface : public UInterface
{
   GENERATED_BODY()
};

class OSELIGHTDETECTION_API IOSELightDetectionInterface
{
   GENERATED_BODY()

public:
   virtual bool CanLightRayHitActor(const FVector& fromLocation, const AActor* actorToIgnore, int& outNumberOfLoSChecksPerformed, float& outMinHitDistance) const = 0;
   virtual float GetCurrentLightIntensityPlusMinimumValue() const = 0;
   virtual float GetActualLightIntensityFromLightSources() const = 0;
};
