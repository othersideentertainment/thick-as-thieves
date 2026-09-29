// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Graphics/TATShadowFadeSpotLightComponent.h"

// tat
#include "Graphics/TATShadowFadeHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATShadowFadeSpotLightComponent)

UTATShadowFadeSpotLightComponent::UTATShadowFadeSpotLightComponent()
{
   CastShadows = false;
   IsPrimaryCrossfadeLight = true;
}

TStructOnScope<FActorComponentInstanceData> UTATShadowFadeSpotLightComponent::GetComponentInstanceData() const
{
   if(IsPrimaryCrossfadeLight)
   {
      return MakeStructOnScope<FActorComponentInstanceData, FTATShadowFadeLightInstanceData>(this);
   }
   else
   {
      return Super::GetComponentInstanceData();
   }
}

void UTATShadowFadeSpotLightComponent::SetupShadowCrossfade(USpotLightComponent* shadowLight)
{
   if (shadowLight)
   {
      ShadowFadeHelpers::CopyMostSpotLightProperties(this, shadowLight);
      ShadowFadeHelpers::SetupShadowLightCrossfade(this, shadowLight);
   }
}

void UTATShadowFadeSpotLightComponent::SetLightShadowCrossfadeDistances(ULocalLightComponent* unshadowedLight, ULocalLightComponent* shadowedLight, float crossfadeDistance, float crossfadeFadeRange)
{
   if (unshadowedLight && shadowedLight)
   {
      ensure(unshadowedLight->IsOwnerRunningUserConstructionScript());
      unshadowedLight->MinDrawDistance = crossfadeDistance;
      unshadowedLight->MinDistanceFadeRange = crossfadeFadeRange;
      ShadowFadeHelpers::SetupShadowLightCrossfade(unshadowedLight, shadowedLight);
   }
}
