// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/SceneVariants/TATSceneAsset.h"

// tat
#include "Variation/MapVariationValidationUtl.h"
#include "Variation/TATMapVariationSeedHelpers.h"
#include "Variation/SceneVariants/TATSceneVariantConfig.h"

// ue5
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSceneAsset)


TPair<UTATSceneVariantConfig*, int32> UTATSceneAsset::SelectRandomVariant(int32 seed, const FGameplayTagContainer& requiredTraits, const FGameplayTagContainer& allowedTraits) const
{
   if (!ensure(Variants.Num()))
   {
      return MakeTuple(nullptr, 0);
   }

   // TODO: can probably pre-compute/generate this seed in the asset
   const int32 derivedSeed = SeedHelpers::MakeSeedForName(GetFName(), seed);

   FRandomStream randomStream(derivedSeed);
   TPair<UTATSceneVariantConfig*, int32> result(nullptr, 0);
   int numFound = 0;

   // https://en.wikipedia.org/wiki/Reservoir_sampling
   const int numVariants = Variants.Num();
   for (int i = 0; i < numVariants; ++i)
   {
      UTATSceneVariantConfig* variant = Variants[i];
      if (!variant || !variant->Matches(requiredTraits, allowedTraits))
      {
         continue;
      }

      if (randomStream.RandRange(0, numFound) == 0)
      {
         result = MakeTuple(variant, i);
      }

      numFound++;
   }

   return result;
}

bool UTATSceneAsset::HasVariant(const UTATSceneVariantConfig* variant) const
{
   return Variants.Contains(variant);
}

bool UTATSceneAsset::HasMatchingVariant(const FGameplayTagContainer& requiredTraits, const FGameplayTagContainer& allowedTraits) const
{
   for (const TObjectPtr<UTATSceneVariantConfig>& variant : Variants)
   {
      if (variant && variant->Matches(requiredTraits, allowedTraits))
      {
         return true;
      }
   }
   return false;
}

bool UTATSceneAsset::HasVariantWith(const FGameplayTagContainer& requiredTraits) const
{
   for (const TObjectPtr<UTATSceneVariantConfig>& variant : Variants)
   {
      if (variant && variant->HasTraits(requiredTraits))
      {
         return true;
      }
   }
   return false;
}

void UTATSceneAsset::PostLoad()
{
   Super::PostLoad();
   _AssignVariantParents();
}

#if WITH_EDITOR
void UTATSceneAsset::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);

   const FName propertyName = propertyChangedEvent.GetPropertyName();
   if (propertyName == GET_MEMBER_NAME_CHECKED(ThisClass, Variants))
   {
      _AssignVariantParents();
   }
}

EDataValidationResult UTATSceneAsset::IsDataValid(FDataValidationContext& context) const
{
   VALIDATE_MISSION_ARRAY_NO_NULL_ENTRIES(Variants);

   return context.GetIssues().Num() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

#endif

void UTATSceneAsset::_AssignVariantParents()
{
   for (UTATSceneVariantConfig* variant : Variants)
   {
      if (variant)
      {
         variant->SetParentScene(this);
      }
   }
}
