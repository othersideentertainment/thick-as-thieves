// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "Components/SceneComponent.h"

#include "TATShadowFadeHelpers.generated.h"

class UPointLightComponent;
class USpotLightComponent;
class ULocalLightComponent;


/// Utility methods for initializing lights for paired shadow crossfades
namespace ShadowFadeHelpers
{
   void CopyMostSpotLightProperties(const USpotLightComponent* source, USpotLightComponent* target);
   void CopyMostPointLightProperties(const UPointLightComponent* source, UPointLightComponent* target);
   void SetupShadowLightCrossfade(ULocalLightComponent* source, ULocalLightComponent* shadowLight);
};

// Used to store data during RerunConstructionScripts
USTRUCT()
struct TAT_API FTATShadowFadeLightInstanceData : public FSceneComponentInstanceData
{
   GENERATED_BODY()
public:

   FTATShadowFadeLightInstanceData() = default;
   explicit FTATShadowFadeLightInstanceData(const USceneComponent* sourceComponent);

   virtual ~FTATShadowFadeLightInstanceData() = default;

   virtual bool ContainsData() const override
   {
      return true;
   }

   virtual void ApplyToComponent(UActorComponent* component, const ECacheApplyPhase cacheApplyPhase) override;
};
