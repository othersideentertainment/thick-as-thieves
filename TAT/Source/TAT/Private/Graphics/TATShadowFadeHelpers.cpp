// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Graphics/TATShadowFadeHelpers.h"

// ue5
#include "Components/LocalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATShadowFadeHelpers)

namespace ShadowFadeHelpers
{

static void CopyMostLightProperties(const ULocalLightComponent* source, ULocalLightComponent* target)
{
   check(source);
   check(target);

   // ULightComponentBase
   target->Intensity = source->Intensity;
   target->LightColor = source->LightColor;
   target->bAffectsWorld = source->bAffectsWorld;
   target->IndirectLightingIntensity = source->IndirectLightingIntensity;
   target->VolumetricScatteringIntensity = source->VolumetricScatteringIntensity;

   // ULightComponent
   target->Temperature = source->Temperature;
   target->bUseTemperature = source->bUseTemperature;
   target->LightFunctionMaterial = source->LightFunctionMaterial;
   target->LightFunctionScale = source->LightFunctionScale;
   target->LightFunctionFadeDistance = source->LightFunctionFadeDistance;
   target->DisabledBrightness = source->DisabledBrightness;

   // ULocalLightComponent
   target->IntensityUnits = source->IntensityUnits;
   target->AttenuationRadius = source->AttenuationRadius;
}

void CopyMostPointLightProperties(const UPointLightComponent* source, UPointLightComponent* target)
{
   target->bUseInverseSquaredFalloff = source->bUseInverseSquaredFalloff;
   target->LightFalloffExponent = source->LightFalloffExponent;
   target->SourceRadius = source->SourceRadius;
   target->SoftSourceRadius = source->SoftSourceRadius;
   target->SourceLength = source->SourceLength;
   CopyMostLightProperties(source, target);
}

void CopyMostSpotLightProperties(const USpotLightComponent* source, USpotLightComponent* target)
{
   target->InnerConeAngle = source->InnerConeAngle;
   target->OuterConeAngle = source->OuterConeAngle;
   CopyMostPointLightProperties(source, target);
}

void SetupShadowLightCrossfade(ULocalLightComponent* source, ULocalLightComponent* shadowLight)
{
   if (source == nullptr || shadowLight == nullptr)
   {
      return;
   }

   const float sourceMinDist = source->MinDrawDistance;
   const float sourceMinFade = source->MinDistanceFadeRange;
   shadowLight->MaxDrawDistance = sourceMinDist + sourceMinFade;
   shadowLight->MaxDistanceFadeRange = sourceMinFade;
   shadowLight->CastShadows = true;

   source->CastShadows = false;

   if (sourceMinFade == 0 && sourceMinDist == 0)
   {
      shadowLight->bAffectsWorld = false;
   }
}

}

FTATShadowFadeLightInstanceData::FTATShadowFadeLightInstanceData(const USceneComponent* sourceComponent)
   : FSceneComponentInstanceData(sourceComponent)
{

}

void FTATShadowFadeLightInstanceData::ApplyToComponent(UActorComponent* component, const ECacheApplyPhase cacheApplyPhase)
{
   // For components that are added in blueprint (i.e. simple-construction-script) rather than c++, the instance
   // overrides of properties are not applies until *after* the UserConstructionScript runs. This makes it
   // impossible for a construction script to read these values in order to copy them to the secondary light.
   // 
   // I am working around this here by explicitly applying the instance properties right before the UCS runs.
   // Some engine classes do this to a more limited degree (e.g. SplineComponent, SplineMeshComponent), but
   // the number of properties to mirror was such that it did not make sense to do it piecemeal.
   //
   // See:
   // https://udn.unrealengine.com/s/question/0D52L00004luoN5SAI/setting-a-value-on-a-component-property-when-actor-placed-in-the-level-editor
   // https://udn.unrealengine.com/s/question/0D52L00004lua35SAA/reading-components-edited-visibleanywhere-properties-in-the-user-construction-script
   // https://udn.unrealengine.com/s/question/0D52L00004lup6ySAA/adjusting-properties-of-a-blueprint-instances-components-in-the-level-does-not-update-references-to-the-properties-in-the-bps-construction-script
   if (cacheApplyPhase == ECacheApplyPhase::PostSimpleConstructionScript)
   {
      FSceneComponentInstanceData::ApplyToComponent(component, ECacheApplyPhase::NonConstructionScript);
   }
   else
   {
      FSceneComponentInstanceData::ApplyToComponent(component, cacheApplyPhase);
   }

   // Since the PostUserConstructionScript diff is done against the archetype, applying
   // the modified properties early will make it erroneously think that the UCS touched more
   // properties that it did, which can interfere with change detection.
   // 
   // Since the primary light is the _source_ of values and is not expected to be written to
   // in the UCS, clearing the modified properties will be a better experience. This could
   // cause minor issues if a UCS does also touch this, but doing a fine-grained diff against
   // the initial snapshot would be a whole thing.
   if (cacheApplyPhase == ECacheApplyPhase::PostUserConstructionScript)
   {
      component->ClearUCSModifiedProperties();
   }
}
