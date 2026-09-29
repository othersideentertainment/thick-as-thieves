// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "OSELightDetectionComponent.h"
#include "TATLightDetectionCharacterComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TAT_API UTATLightDetectionCharacterComponent : public UOSELightDetectionComponent
{
   GENERATED_BODY()

public:
   virtual float GetMinimumLightIntensity() const override;

protected:
   UPROPERTY(EditAnywhere)
   UCurveFloat* _MovementSpeedToMinimumLightIntensity { nullptr };
};
