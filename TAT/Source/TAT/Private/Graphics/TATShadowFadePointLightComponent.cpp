// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Graphics/TATShadowFadePointLightComponent.h"

// tat
#include "Graphics/TATShadowFadeHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATShadowFadePointLightComponent)

UTATShadowFadePointLightComponent::UTATShadowFadePointLightComponent()
{
   CastShadows = false;
   IsPrimaryCrossfadeLight = true;
}

TStructOnScope<FActorComponentInstanceData> UTATShadowFadePointLightComponent::GetComponentInstanceData() const
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

void UTATShadowFadePointLightComponent::SetupShadowCrossfade(UPointLightComponent* shadowLight)
{
   if (shadowLight)
   {
      ShadowFadeHelpers::CopyMostPointLightProperties(this, shadowLight);
      ShadowFadeHelpers::SetupShadowLightCrossfade(this, shadowLight);
   }
}
