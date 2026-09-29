// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/SceneVariants/TATSceneVariantConfig.h"

// tat
#include "Variation/MapVariationValidationUtl.h"

// ue5
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSceneVariantConfig)



const FTATSceneSpawnerOverride FTATSceneSpawnerOverride::Default(ETATSceneSpawnerOverrideType::Default);
const FTATSceneSpawnerOverride FTATSceneSpawnerOverride::Never(ETATSceneSpawnerOverrideType::Never);

const FTATSceneSpawnerOverride& UTATSceneVariantConfig::ResolveSpawner(FGameplayTag key) const
{
   if (const FTATSceneSpawnerOverride* found = _spawnerOverrides.Find(key))
   {
      return *found;
   }

   if (_simpleFlags.HasTagExact(key))
   {
      return FTATSceneSpawnerOverride::Default;
   }

   return FTATSceneSpawnerOverride::Never;
}

bool UTATSceneVariantConfig::ResolveBool(FGameplayTag key) const
{
   if (_simpleFlags.HasTagExact(key))
   {
      return true;
   }

   // Allow flags to read from spawner overrides for easier interop
   if (const FTATSceneSpawnerOverride* found = _spawnerOverrides.Find(key))
   {
      return found->Type != ETATSceneSpawnerOverrideType::Never;
   }

   return false;
}

bool UTATSceneVariantConfig::Matches(const FGameplayTagContainer& requiredTraits, const FGameplayTagContainer& allowedTraits) const
{
   return _sceneTraits.HasAll(requiredTraits) && allowedTraits.HasAll(_sceneTraits);
}

bool UTATSceneVariantConfig::HasTraits(const FGameplayTagContainer& requiredTraits) const
{
   return _sceneTraits.HasAll(requiredTraits);
}

const FText& UTATSceneVariantConfig::GetClueLocationName() const
{
   // Should be N/A for this type of clue source, can add something later if needed
   return FText::GetEmpty();
}

const FGameplayTag& UTATSceneVariantConfig::GetClueLocationTag() const
{
   return _clueLocationTag;
}

#if WITH_EDITOR
EDataValidationResult UTATSceneVariantConfig::IsDataValid(FDataValidationContext& context) const
{
   // Only adding this because I have accidentally done this a couple times setting up test data
   if (_spawnerOverrides.Contains(FGameplayTag()))
   {
      VALIDATE_ADDERROR(TEXT("SpawnerOverrides contains an empty key tag"));
   }

   return context.GetIssues().Num() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
