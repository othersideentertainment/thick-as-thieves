// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "Components/PointLightComponent.h"

#include "TATShadowFadePointLightComponent.generated.h"

/// Helper component for initializing lights for paired shadow crossfades
///
/// This should be used for the main, unshadowed, light, although that can be
/// disabled for convenience.
UCLASS(ClassGroup = Lights, meta = (BlueprintSpawnableComponent))
class TAT_API UTATShadowFadePointLightComponent : public UPointLightComponent
{
	GENERATED_BODY()

public:
   UTATShadowFadePointLightComponent();

   //~ Begin UActorComponent Interface.
   virtual TStructOnScope<FActorComponentInstanceData> GetComponentInstanceData() const override;
   //~ End UActorComponent Interface.

   // Should be called in construction script to set up the secondary light
   // to mirror this one, and have its shadow fade-out when this lights min
   // distance starts to fade in.
   UFUNCTION(BlueprintCallable, Category = "Light|TAT")
   void SetupShadowCrossfade(UPointLightComponent* shadowLight);

public:
   UPROPERTY(EditDefaultsOnly, Category=Light)
   bool IsPrimaryCrossfadeLight;
};
