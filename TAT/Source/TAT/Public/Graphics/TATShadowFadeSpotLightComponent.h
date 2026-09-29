// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "Components/SpotLightComponent.h"

#include "TATShadowFadeSpotLightComponent.generated.h"

/// Helper component for initializing lights for paired shadow crossfades
///
/// This should be used for the main, unshadowed, light, although that can be
/// disabled for convenience.
/// 
/// TODO: Try method that spawns secondary component directly? (Just need to make sure it spawns just like a normal UCS-spawned component)
UCLASS(ClassGroup = Lights, meta = (BlueprintSpawnableComponent))
class TAT_API UTATShadowFadeSpotLightComponent : public USpotLightComponent
{
	GENERATED_BODY()

public:
   UTATShadowFadeSpotLightComponent();

   //~ Begin UActorComponent Interface.
   virtual TStructOnScope<FActorComponentInstanceData> GetComponentInstanceData() const override;
   //~ End UActorComponent Interface.

   // Should be called in construction script to set up the secondary light
   // to mirror this one, and have its shadow fade-out when this lights min
   // distance starts to fade in.
   UFUNCTION(BlueprintCallable, Category = "Light|TAT")
   void SetupShadowCrossfade(USpotLightComponent* shadowLight);

   // Meant to be called in a constructions script
   // Only sets values derived from the crossfade distance and range, so
   // does not risk fighting with other construction scripts
   //
   // Does not mirror other parameters between lights
   //
   // (Doesn't specifically belong to this subclass, but didn't want to make a uobject class just for this)
   UFUNCTION(BlueprintCallable, Category = "Light|TAT")
   static void SetLightShadowCrossfadeDistances(ULocalLightComponent* unshadowedLight, ULocalLightComponent* shadowedLight, float crossfadeDistance, float crossfadeFadeRange);

public:
   UPROPERTY(EditDefaultsOnly, Category=Light)
   bool IsPrimaryCrossfadeLight;
};
