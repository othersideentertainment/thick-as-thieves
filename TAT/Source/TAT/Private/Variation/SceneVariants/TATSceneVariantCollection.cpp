// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/SceneVariants/TATSceneVariantCollection.h"

// tat
#include "Variation/MapVariationValidationUtl.h"
#include "Variation/SceneVariants/TATSceneVariantConfig.h"
#include "Variation/SceneVariants/TATSceneRequirement.h"

// ue5
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSceneVariantCollection)


const FTATSceneVariantCollection& FTATSceneVariantCollection::Empty()
{
   static const FTATSceneVariantCollection sEmpty;
   return sEmpty;
}

const FTATSceneSpawnerOverride& FTATSceneVariantCollection::ResolveSpawner(const FTATSceneRequirement& requirement) const
{
   switch (requirement.Type)
   {
   case ETATSceneRequirementType::None:
      return FTATSceneSpawnerOverride::Default;
   case ETATSceneRequirementType::RequireVariant:
      return _variants.Contains(requirement.Variant) ? FTATSceneSpawnerOverride::Default : FTATSceneSpawnerOverride::Never;
   case ETATSceneRequirementType::RequireSceneElement:
   {
      if (const UTATSceneVariantConfig* variant = _sceneToConfig.FindRef(requirement.Scene))
      {
         return variant->ResolveSpawner(requirement.ElementTag);
      }
      return FTATSceneSpawnerOverride::Never;
   }
   default:
      checkNoEntry();
      return FTATSceneSpawnerOverride::Never;
   }
}

bool FTATSceneVariantCollection::ResolveBool(const FTATSceneRequirement& requirement) const
{
   switch (requirement.Type)
   {
   case ETATSceneRequirementType::None:
      return true;
   case ETATSceneRequirementType::RequireVariant:
      return _variants.Contains(requirement.Variant);
   case ETATSceneRequirementType::RequireSceneElement:
   {
      if (const UTATSceneVariantConfig* variant = _sceneToConfig.FindRef(requirement.Scene))
      {
         return variant->ResolveBool(requirement.ElementTag);
      }
      return false;
   }
   default:
      checkNoEntry();
      return false;
   }
}

const UTATSceneVariantConfig* FTATSceneVariantCollection::FindVariantForScene(const UTATSceneAsset* scene) const
{
	return _sceneToConfig.FindRef(scene);
}

void FTATSceneVariantCollection::Reset()
{
   _variants.Reset();
   _sceneToConfig.Reset();
}

void FTATSceneVariantCollection::AddVariant(const UTATSceneAsset* scene, const UTATSceneVariantConfig* variant)
{
   if (variant == nullptr) return;
   check(scene);

   _variants.Add(variant);
   _sceneToConfig.Add(scene, variant);
}

void FTATSceneVariantCollection::WriteToMessageLog(FMessageLog& msgLog) const
{
   for (const UTATSceneVariantConfig* variant : _variants)
   {
      msgLog.Info(FText::FromString(TEXT("Using SceneVariant: ")))
         ->AddToken(FUObjectToken::Create(variant));
   }
}

void FTATSceneVariantCollection::WriteToLog() const
{
#if !NO_LOGGING
   for (const UTATSceneVariantConfig* variant : _variants)
   {
      UE_LOG(LogTATMapVariation, Log, TEXT("Using SceneVariant: %s"), *variant->GetName());
   }
#endif
}

void FTATSceneVariantCollection::WriteToOutput(FOutputDevice& outputDevice) const
{
#if ALLOW_DEBUG_FILES
   for (const UTATSceneVariantConfig* variant : _variants)
   {
      outputDevice.Logf(TEXT("Using SceneVariant: %s"), *variant->GetName());
   }
#endif
}

FString FTATSceneVariantCollection::ToCompactString() const
{
   FString result;
   for (const UTATSceneVariantConfig* variant : _variants)
   {
      if (!result.IsEmpty())
      {
         result.AppendChar(TEXT(','));
      }
      variant->AppendName(result);
   }
   return result;
}

bool FTATSceneVariantIndices::NetSerialize(FArchive& ar, UPackageMap* packageMap, bool& outSuccess)
{
   constexpr int kMaxScenes = 200; //< Likely overkill, but what are a few bits between friends
   ar << _initialized;
   outSuccess = true;
   if(_initialized)
   {
      outSuccess = SafeNetSerializeTArray_Default<kMaxScenes>(ar, _variantSelections);
   }
	return outSuccess;
}
