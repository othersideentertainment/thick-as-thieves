// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/SceneVariants/TATSceneSet.h"

// tat
#include "Variation/MapVariationValidationUtl.h"
#include "Variation/TATMapVariationSeedHelpers.h"
#include "Variation/SceneVariants/TATSceneAsset.h"
#include "Variation/SceneVariants/TATSceneVariantConfig.h"
#include "Variation/SceneVariants/TATSceneVariantCollection.h"

// ue5
#include "Algo/Count.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSceneSet)


void UTATSceneSetAsset::SelectVariants(const FTATSceneVariantSelectionParams& params, TFunctionRef<void(const UTATSceneAsset*, const UTATSceneVariantConfig*, int32)> handler) const
{
   TRACE_CPUPROFILER_EVENT_SCOPE(UTATSceneSetAsset::SelectVariants)
   // using memstack for temp buffers
   FMemStackBase& memStack = FMemStack::Get();
   FMemMark mark(memStack);

   // array of remaining scenes, and their index in the scene array (could have also just been the index)
   TArray<TPair<const UTATSceneAsset*, int32>, TMemStackAllocator<>> remainingScenes;
   remainingScenes.Reserve(Scenes.Num());
   for (int i = 0; i < Scenes.Num(); ++i)
   {
      remainingScenes.Emplace(Scenes[i], i);
   }

   TArray<TPair<const UTATSceneAsset*, int32>, TMemStackAllocator<>> candidateScenes;
   candidateScenes.Reserve(Scenes.Num());

   // array of chosen scene variants and their indexes, indexed by the original index of the scene in the array
   TArray<TPair<const UTATSceneVariantConfig*, int32>, TMemStackAllocator<>> chosenVariants;
   chosenVariants.SetNumZeroed(Scenes.Num());

   // apply any forced variants first
   for(const UTATSceneVariantConfig* variant : params.VariantOverrides)
   {
      for (int i = 0; i < remainingScenes.Num(); ++i)
      {
         const UTATSceneAsset* scene = remainingScenes[i].Key;
         check(scene);
         const int32 variantIndex = scene->Variants.IndexOfByKey(variant);

         if (variantIndex >= 0)
         {
            const int sceneIndex = remainingScenes[i].Value;
            chosenVariants[sceneIndex] = MakeTuple(variant, variantIndex);
            remainingScenes.RemoveAtSwap(i);
            break;
         }
      }
   }

   const int32 seed = params.Seed;
   // TODO: embed seen in asset?
   const int32 derivedSeed = SeedHelpers::MakeSeedForName(GetFName(), seed);
   TArray<FTATScenePhaseSummary>* outPhaseSummaries = params.OutPhaseSummary;

   const FTATSceneSetOverride* const setOverride = params.SceneSetOverrides ? params.SceneSetOverrides->Find(this) : nullptr;
   TConstArrayView<FTATSceneVariantSelectionPhase> selectionPhases = (setOverride && setOverride->OverrideSelectionPhases) ? setOverride->SelectionPhases : SelectionPhases;
   
   FGameplayTagContainer allowedTraits;
   for (const FGameplayTag& tag : params.ExtraAllowedTraits)
   {
      allowedTraits.AddTagFast(tag);
   }
   if(setOverride)
   {
      allowedTraits.AppendTags(setOverride->ExtraSceneTraits);
   }
   for (const FTATSceneVariantSelectionPhase& phase : selectionPhases)
   {
      allowedTraits.AddTagFast(phase.RequiredTrait);
   }
   allowedTraits.FillParentTags();

   // choose for phases first
   for (const FTATSceneVariantSelectionPhase& phase : selectionPhases)
   {
      const FGameplayTagContainer& requiredTags = phase.RequiredTrait.GetSingleTagContainer();

      candidateScenes.Reset();
      for (const TPair<const UTATSceneAsset*, int32>& scenePair : remainingScenes)
      {
         if (scenePair.Key->HasMatchingVariant(requiredTags, allowedTraits))
         {
            candidateScenes.Add(scenePair);
         }
      }

      const int32 phaseSeed = SeedHelpers::MakeSeedForName(phase.RequiredTrait.GetTagName(), derivedSeed);
      FRandomStream randomStream(phaseSeed);

      const int32 numToChoose = randomStream.RandRange(phase.SceneCount.Min, phase.SceneCount.Max);
      const int32 numAlreadyChosen = Algo::CountIf(chosenVariants, [&requiredTags](const TPair<const UTATSceneVariantConfig*, int32>& p) { return p.Key && p.Key->HasTraits(requiredTags); });
      const int32 numAdditionalToChoose = FMath::Clamp(numToChoose - numAlreadyChosen, 0, candidateScenes.Num());
      const int32 totalChosen = numAlreadyChosen + numAdditionalToChoose;
      UE_LOG(LogTATMapVariation, Verbose, TEXT("%s[%s] chosen %d scenes"), *GetName(), *phase.RequiredTrait.ToString(), totalChosen);
      UE_CLOG(totalChosen < numToChoose && params.AllowWarningLogs, LogTATMapVariation, Warning, TEXT("%s[%s] chose fewer scenes than desired (%d < %d)"), *GetName(), *phase.RequiredTrait.ToString(), totalChosen, numToChoose);

      for (int i = 0; i < numAdditionalToChoose; ++i)
      {
         const int32 candidateIndex = randomStream.RandHelper(candidateScenes.Num());
         TPair<const UTATSceneAsset*, int32> scenePair = candidateScenes[candidateIndex];
         candidateScenes.RemoveAtSwap(candidateIndex);
         remainingScenes.RemoveSingleSwap(scenePair);

         chosenVariants[scenePair.Value] = scenePair.Key->SelectRandomVariant(seed, requiredTags, allowedTraits);
      }

      // exclude trait from later selection
      allowedTraits.RemoveTag(phase.RequiredTrait);

      if (outPhaseSummaries)
      {
         FTATScenePhaseSummary& summary = outPhaseSummaries->Emplace_GetRef();
         summary.SceneSet = this;
         summary.PhaseTag = phase.RequiredTrait;
         summary.NumDesired = numToChoose;
         summary.NumChosen = totalChosen;
         summary.Min = phase.SceneCount.Min;
         summary.Max = phase.SceneCount.Max;
      }
   }

   // Choose randomly from remaining scenes
   for (TPair<const UTATSceneAsset*, int32> scenePair : remainingScenes)
   {
      chosenVariants[scenePair.Value] = scenePair.Key->SelectRandomVariant(seed, FGameplayTagContainer::EmptyContainer, allowedTraits);
   }

   for (int i = 0; i < Scenes.Num(); ++i)
   {
      TPair<const UTATSceneVariantConfig*, int32> chosen = chosenVariants[i];
      handler(Scenes[i], chosen.Key, chosen.Value);
   }
}

int32 UTATSceneSetAsset::EmitVariantsForIndices(TConstArrayView<uint8> indices, TFunctionRef<void(const UTATSceneAsset*, const UTATSceneVariantConfig*)> handler) const
{
   int sceneIndex = 0;
   for (const UTATSceneAsset* scene : Scenes)
   {
      check(scene);

      if (sceneIndex >= indices.Num())
      {
         UE_LOG(LogTATMapVariation, Warning, TEXT("EmitVariantsForIndices: Fewer indices than scenes"));
         break;
      }

      const int32 chosenVariantIndex = indices[sceneIndex];
      if (scene->Variants.IsValidIndex(chosenVariantIndex))
      {
         handler(scene, scene->Variants[chosenVariantIndex]);
      }

      ++sceneIndex;
   }

   return sceneIndex;
}

void UTATSceneSetAsset::ForEachVariant(TFunctionRef<void(const UTATSceneVariantConfig*)> handler) const
{
   for (const UTATSceneAsset* scene : Scenes)
   {
      check(scene);

      for (const UTATSceneVariantConfig* variant : scene->Variants)
      {
         if (variant)
         {
            handler(variant);
         }
      }
   }
}

bool UTATSceneSetAsset::HasScene(const UTATSceneAsset* scene) const
{
   return Scenes.Contains(scene);
}

bool UTATSceneSetAsset::HasVariant(const UTATSceneVariantConfig* variant) const
{
   return Scenes.ContainsByPredicate([variant](const UTATSceneAsset* scene) { return scene && scene->HasVariant(variant); });
}

const UTATSceneAsset* UTATSceneSetAsset::FindSceneForVariant(const UTATSceneVariantConfig* variant) const
{
   const TObjectPtr<UTATSceneAsset>* found = Scenes.FindByPredicate([variant](const UTATSceneAsset* scene) { return scene && scene->HasVariant(variant); });
   return found ? *found : nullptr;
}

#if WITH_EDITOR
EDataValidationResult UTATSceneSetAsset::IsDataValid(FDataValidationContext& context) const
{
   VALIDATE_MISSION_ARRAY_NO_NULL_ENTRIES(Scenes);

   // This doesn't guarantee anything after more than one phase, but is an easy floor
   for (const FTATSceneVariantSelectionPhase& phase : SelectionPhases)
   {
      if (phase.SceneCount.Max > Scenes.Num())
      {
         VALIDATE_ADDERROR(FString::Printf(TEXT("Phase %s requires more scenes than are in the set (%d < %d)"), *phase.RequiredTrait.ToString(), Scenes.Num(), phase.SceneCount.Max));
         continue;
      }

      const int possibleScenes = Algo::CountIf(Scenes, [&phase](const UTATSceneAsset* s) { return s && s->HasVariantWith(phase.RequiredTrait.GetSingleTagContainer()); });
      if (phase.SceneCount.Min > possibleScenes)
      {
         VALIDATE_ADDERROR(FString::Printf(TEXT("Phase %s requires more scenes than are in the set (%d < %d)"), *phase.RequiredTrait.ToString(), possibleScenes, phase.SceneCount.Min));
      }
   }

   return context.GetIssues().Num() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

void UTATSceneSetAsset::ValidateDuplicates(TConstArrayView<TObjectPtr<UTATSceneSetAsset>> sceneSets, const UObject* owner, FDataValidationContext& context)
{
   check(owner);

   // it would be nice if the errors could be more local, but that is trickier without a more rigid tag structure
   TArray<const UTATSceneSetAsset*> visitedSets;
   TMap<const UTATSceneAsset*, const UTATSceneSetAsset*> visitedScenes;
   TMap<const UTATSceneVariantConfig*, const UTATSceneAsset*> visitedVariants;

   for (const UTATSceneSetAsset* sceneSet : sceneSets)
   {
      if(sceneSet == nullptr) continue; // other validation will catch this, but don't crash
      if (visitedSets.Contains(sceneSet))
      {
         context.AddError(FText::FromString(FString::Format(TEXT("[{0}] SceneSet {1} is contained multiple times"),
            {owner->GetName(), sceneSet->GetName()})));
         continue;
      }
      visitedSets.Add(sceneSet);

      for (const UTATSceneAsset* scene : sceneSet->Scenes)
      {
         if (scene == nullptr) continue; // other validation will catch this, but don't crash
         if (const UTATSceneSetAsset* otherSet = visitedScenes.FindRef(scene))
         {
            context.AddError(FText::FromString(FString::Format(TEXT("[{0}] Scene {1} is contained multiple times (From {2} and {3})"),
               { owner->GetName(), scene->GetName(), sceneSet->GetName(), otherSet->GetName()})));
            continue;
         }
         visitedScenes.Add(scene, sceneSet);

         for (const UTATSceneVariantConfig* variant : scene->Variants)
         {
            if (variant == nullptr) continue; // other validation will catch this, but don't crash
            if (const UTATSceneAsset* otherScene = visitedVariants.FindRef(variant))
            {
               context.AddError(FText::FromString(FString::Format(TEXT("[{0}] Variant {1} is contained multiple times (From {2} and {3})"),
                  { owner->GetName(), variant->GetName(), scene->GetName(), otherScene->GetName() })));
               continue;
            }
            visitedVariants.Add(variant, scene);
         }
      }
   }
}
#endif
