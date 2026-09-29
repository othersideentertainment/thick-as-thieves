// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/SceneVariants/TATSceneSelection.h"

// tat
#include "Variation/MapVariationValidationUtl.h"
#include "Variation/TATMapVariationSeedHelpers.h"
#include "Variation/SceneVariants/TATSceneAsset.h"
#include "Variation/SceneVariants/TATSceneSet.h"
#include "Variation/SceneVariants/TATSceneVariantConfig.h"
#include "Variation/SceneVariants/TATSceneVariantCollection.h"

// ue5
#include "Algo/Count.h"


namespace TATSceneSelection
{
   static void SimpleSelectVariants(TConstArrayView<TObjectPtr<UTATSceneSetAsset>> sceneSets, const FTATSceneVariantSelectionParams& params, TFunctionRef<void(const UTATSceneAsset*, const UTATSceneVariantConfig*, int32)> handler)
   {
      for (TObjectPtr<UTATSceneSetAsset> sceneSet : sceneSets)
      {
         check(sceneSet);
         sceneSet->SelectVariants(params, handler);
      }
   }

   static TArray<TObjectPtr<UTATSceneVariantConfig>> SelectLimitedTraits(TConstArrayView<TObjectPtr<UTATSceneSetAsset>> sceneSets, const FTATSceneVariantSelectionParams& params)
   {
      // NOTE: This resembles the logic for phase selection in SceneSet::SelectVariants, but working across scene sets
      // using memstack for temp buffers
      FMemStackBase& memStack = FMemStack::Get();
      FMemMark mark(memStack);

      const int32 totalSceneCount = Algo::TransformAccumulate(sceneSets, [](const UTATSceneSetAsset* sceneSet) { return sceneSet->Scenes.Num();}, 0);

      TArray<const UTATSceneAsset*, TMemStackAllocator<>> remainingScenes;
      remainingScenes.Reserve(totalSceneCount);
      for (TObjectPtr<UTATSceneSetAsset> sceneSet : sceneSets)
      {
         check(sceneSet);
         for(const UTATSceneAsset* sceneAsset : sceneSet->Scenes)
         {
            remainingScenes.Emplace(sceneAsset);
         }
      }
      
      TArray<const UTATSceneAsset*, TMemStackAllocator<>> candidateScenes;
      candidateScenes.Reserve(totalSceneCount);

      // array of chosen scene variants
      TArray<TObjectPtr<UTATSceneVariantConfig>> chosenVariants;
      chosenVariants.Reserve(totalSceneCount);

      for(UTATSceneVariantConfig* variant : params.VariantOverrides)
      {
         for (int i = 0; i < remainingScenes.Num(); ++i)
         {
            const UTATSceneAsset* scene = remainingScenes[i];
            check(scene);
            const int32 variantIndex = scene->Variants.IndexOfByKey(variant);

            if (variantIndex >= 0)
            {
               chosenVariants.Add(variant);
               remainingScenes.RemoveAtSwap(i);
               break;
            }
         }
      }

      FGameplayTagContainer allowedTraits;
      for (const FGameplayTag& tag : params.ExtraAllowedTraits)
      {
         allowedTraits.AddTagFast(tag);
      }

      for (const FTATSceneTraitWithLimit& trait : params.LimitedTraits)
      {
         const FGameplayTagContainer& requiredTags = trait.Trait.GetSingleTagContainer();

         candidateScenes.Reset();
         allowedTraits.AddTagFast(trait.Trait);
         for (const UTATSceneAsset*& scene : remainingScenes)
         {
            if (scene->HasMatchingVariant(requiredTags, allowedTraits))
            {
               candidateScenes.Add(scene);
            }
         }
         
         const int32 traitSeed = SeedHelpers::MakeSeedForName(trait.Trait.GetTagName(), params.Seed);
         FRandomStream randomStream(traitSeed);

         const int32 numToChoose = trait.Limit;
         const int32 numAlreadyChosen = Algo::CountIf(chosenVariants, [&requiredTags](const UTATSceneVariantConfig* p) { return p->HasTraits(requiredTags); });
         const int32 numAdditionalToChoose = FMath::Clamp(numToChoose - numAlreadyChosen, 0, candidateScenes.Num());
         const int32 totalChosen = numAlreadyChosen + numAdditionalToChoose;
         UE_LOG(LogTATMapVariation, Verbose, TEXT("[%s] chosen %d scenes"), *trait.Trait.ToString(), totalChosen);

         for (int i = 0; i < numAdditionalToChoose; ++i)
         {
            const int32 candidateIndex = randomStream.RandHelper(candidateScenes.Num());
            const UTATSceneAsset* scene = candidateScenes[candidateIndex];
            candidateScenes.RemoveAtSwap(candidateIndex);
            remainingScenes.RemoveSingleSwap(scene);

            UTATSceneVariantConfig* variant = scene->SelectRandomVariant(params.Seed, requiredTags, allowedTraits).Key;
            check(variant);
            chosenVariants.Add(variant);
         }

         // remove trait from allowed traits afterwards
         allowedTraits.RemoveTag(trait.Trait);

         if (params.OutPhaseSummary)
         {
            FTATScenePhaseSummary& summary = params.OutPhaseSummary->Emplace_GetRef();
            summary.PhaseTag = trait.Trait;
            summary.NumDesired = numToChoose;
            summary.NumChosen = totalChosen;
            summary.Min = trait.Limit;
            summary.Max = trait.Limit;
         }
      }
      return chosenVariants;
   }
}

void TATSceneSelection::SelectVariants(TConstArrayView<TObjectPtr<UTATSceneSetAsset>> sceneSets, const FTATSceneVariantSelectionParams& params, TFunctionRef<void(const UTATSceneAsset*, const UTATSceneVariantConfig*, int32)> handler)
{
   // If no limited traits, do it the simple way
   if (params.LimitedTraits.IsEmpty())
   {
      SimpleSelectVariants(sceneSets, params, handler);
      return;
   }

   // If there are limited traits, choose them first and feed them in as overrides
   // NOTE: I considered allocating this result via the memstack like everything else here, but worried
   //       that reasoning about that non-locally may not be ideal for maintenance relative to the perf gain.
   TArray<TObjectPtr<UTATSceneVariantConfig>> overridesFromTraits = SelectLimitedTraits(sceneSets, params);
   FTATSceneVariantSelectionParams newParams = params;
   newParams.VariantOverrides = overridesFromTraits;
   SimpleSelectVariants(sceneSets, newParams, handler);
}
