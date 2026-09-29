// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Components/ActorComponent.h"

#include "OSELightDetectionComponent.generated.h"


class IOSELightDetectionInterface;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class OSELIGHTDETECTION_API UOSELightDetectionComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UOSELightDetectionComponent();
   virtual void InitializeComponent() override;
   virtual void UninitializeComponent() override;

   const UPrimitiveComponent* GetPrimitiveComponent() const;
   IOSELightDetectionInterface* GetLightDetectionInterface() const;
   
   void SetLightDetectionValues(float actualLightIntensity, float lightIntensityPlusMinimum, FLinearColor color);
   bool CanHandleCalculation() const;
   FTransform GetLightDetectionTransform() const;
   float GetActualLightIntensityFromLightSources() const { return _ActualLightIntensityFromLightSources; };
   float GetCurrentLightIntensityPlusMinimumValue() const { return _CurrentLightIntensityPlusMinimumValue; };
   virtual float GetMinimumLightIntensity() const { return 0.f; }

private:
   UPROPERTY()
   ACharacter* _CharacterOwner { nullptr };
   
   UPROPERTY()
   TScriptInterface<IOSELightDetectionInterface> _LightDetectionInterface { nullptr };

   float _ActualLightIntensityFromLightSources {0.f};
   float _CurrentLightIntensityPlusMinimumValue {0.f};
   
   FLinearColor _CurrentLightColor {FColor::Black};
};
