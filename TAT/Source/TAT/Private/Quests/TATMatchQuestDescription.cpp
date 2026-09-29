// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/TATMatchQuestDescription.h"

// tat
#include "Lockpicking/TATCombinationHelpers.h"
#include "Lockpicking/TATLockCombinationName.h"
#include "Lockpicking/TATCombinationScrape.h"
#include "Variation/TATMapVariationSeedHelpers.h"
#include "Quests/TATQuestFormatParamSource.h"

// ue
#include "StructUtils/InstancedStruct.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/DataValidation.h"
#include "UObject/AssetRegistryTagsContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATMatchQuestDescription)

namespace MatchQuestHelpers
{
   static void AddLockCombinationsToFormatParams(const TMap<FString, FTATLockCombinationNameRef>& combinations, FTATClueFormatParams& formatParams, int32 mapSeed)
   {
      for(const TPair<FString, FTATLockCombinationNameRef>& pair : combinations)
      {
         formatParams.Add(pair.Key, CombinationHelpers::FormatCombinationFromName(pair.Value.Name, mapSeed));
      }
   }

   static void AddSourcesToFormatParams(TConstArrayView<FInstancedStruct> sources, FTATClueFormatParams& formatParams, int32 mapSeed)
   {
      FTATQuestFormatParamSource::FParams params = { .MapSeed = mapSeed };
      for(const FInstancedStruct& source : sources)
      {
         if(source.IsValid())
         {
            source.Get<FTATQuestFormatParamSource>().AddTextReplacement(params,
               [&formatParams](const FString& key, const FText& text) { formatParams.Add(key, text); });
         }
      }
   }

#if WITH_EDITOR
   static void ValidateParamSources(TConstArrayView<FInstancedStruct> sources, TFunctionRef<void (const FText&)> reportError)
   {
      for(int i = 0; i < sources.Num(); ++i)
      {
         const FInstancedStruct& source = sources[i];
         if(source.IsValid())
         {
            source.Get<FTATQuestFormatParamSource>().Validate([reportError, i](const FText& message)
            {
               reportError(FText::FormatOrdered(INVTEXT("FormatParamSources[{0}]: {1}"), i, message));
            });
         }
         else
         {
            reportError(FText::FormatOrdered(INVTEXT("FormatParamSources[{0}]: No source set"), i));
         }
      }
   }
#endif
}

void UTATMatchQuestDescription::GenerateChoices(FTATQuestChoicePlan& result, const FTATQuestChoiceParams& params) const
{
   TRACE_CPUPROFILER_EVENT_SCOPE(UTATMatchQuestDescription::GenerateChoices)
   result.Reset();
   result.ForcedSceneVariants.Reserve(Algo::TransformAccumulate(Choices,[](const FTATQuestChoice& c) { return c.NumToChoose.Max; },
      params.VariantOverrides.Num() + ForcedSceneVariants.Num()));
   result.ForcedSceneVariants.Append(params.VariantOverrides);
   result.ForcedSceneVariants.Append(ForcedSceneVariants);

   // add common lock combinations and format params
   MatchQuestHelpers::AddLockCombinationsToFormatParams(LockCombinations, result.FormatParams, params.Seed);
   MatchQuestHelpers::AddSourcesToFormatParams(FormatParamSources, result.FormatParams, params.Seed);

   TArray<int32, TInlineAllocator<32>> possibleOptionIndices;
   TArray<const UTATSceneAsset*, TInlineAllocator<32>> scenesForOptionIndex;
   TArray<const UTATSceneAsset*, TInlineAllocator<32>> involvedScenes;
   TArray<const UTATSceneVariantConfig*, TInlineAllocator<64>> excludedVariants;
   for(const UTATSceneVariantConfig* sceneVariant : result.ForcedSceneVariants)
   {
      if(const UTATSceneAsset* parentScene = params.FindParentScene(sceneVariant))
      {
         involvedScenes.AddUnique(parentScene);
      }
   }
   
   const int32 derivedSeed = SeedHelpers::MakeSeedForName(GetFName(), params.Seed);
   // CONSIDER: The randomization of each choice would ideally use its own stream so that
   //           it changes less chaotically when content changes. But choices don't have
   //           a natural identifier, and I don't want to add one _just_ for this.
   FRandomStream randomStream(derivedSeed);

   auto addChosenOption = [&result, seed = params.Seed](const FTATQuestChoiceOption& option)
   {
      result.ChoiceTags.AppendTags(option.AddedChoiceTags);
      MatchQuestHelpers::AddLockCombinationsToFormatParams(option.LockCombinations, result.FormatParams, seed);
      MatchQuestHelpers::AddSourcesToFormatParams(option.FormatParamSources, result.FormatParams, seed);
   };
   
   for(const FTATQuestChoice& choice : Choices)
   {
      const int32 numToChoose = randomStream.RandRange(choice.NumToChoose.Min, choice.NumToChoose.Max);
      int32 numChosen = 0;
      possibleOptionIndices.Reset();
      scenesForOptionIndex.Reset();

      // populate valid options for choices
      for(int i = 0; i < choice.Options.Num(); i++)
      {
         const FTATQuestChoiceOption& option = choice.Options[i];
         const UTATSceneVariantConfig* variant = option.SceneVariant;
         const UTATSceneAsset* parentScene = variant ? params.FindParentScene(variant) : nullptr;
         scenesForOptionIndex.Add(parentScene);

         // skip if in excluded variants
         if(variant && excludedVariants.Contains(variant))
         {
            continue;
         }

         if(variant && result.ForcedSceneVariants.Contains(variant))
         {
            // If this variant is forced, force the choice as well
            // NOTE: (2024-11-12) Per discussion, it is valid for now to assume that a variant being forced
            //       implies that any choice that would choose that variant should also be forced. May
            //       revisit if we find ourselves wanting to have distinct choices in a quest that use
            //       the same choice.
            numChosen++;
            addChosenOption(option);
         }
         else if(!variant || !involvedScenes.Contains(parentScene))
         {
            // If something already uses the same scene, it cannot be used
            possibleOptionIndices.Add(i);
         }
      }

      // Choose options
      while(numChosen < numToChoose && possibleOptionIndices.Num() > 0)
      {
         const int32 indexIndexToRemove = randomStream.RandHelper(possibleOptionIndices.Num());
         const int32 chosenOptionIndex = possibleOptionIndices[indexIndexToRemove];
         {
            const FTATQuestChoiceOption& option = choice.Options[chosenOptionIndex];
            addChosenOption(option);
            if(option.SceneVariant)
            {
               result.ForcedSceneVariants.Add(option.SceneVariant);
            }
         }
         const UTATSceneAsset* parentScene = scenesForOptionIndex[chosenOptionIndex];
         if(parentScene)
         {
            check(!involvedScenes.Contains(parentScene));
            involvedScenes.Add(parentScene);
         }
         ++numChosen;
         possibleOptionIndices.RemoveAtSwap(indexIndexToRemove);

         // Remove any options using the same scene
         if(parentScene && numChosen < numToChoose && possibleOptionIndices.Num() > 0)
         {
            possibleOptionIndices.RemoveAllSwap([&](int32 optionIndex) { return parentScene == scenesForOptionIndex[optionIndex]; });
         }
      }

      // exclude any unselected variants from being selected again in a later choice
      // (notice that this does not include ones excluded because of scenes, but those would be excluded anyways)
      for(int32 notChosenIndex : possibleOptionIndices)
      {
         if(const UTATSceneVariantConfig* variant = choice.Options[notChosenIndex].SceneVariant)
         {
            excludedVariants.Add(variant);
         }
      }
   }
}

#if WITH_EDITOR
void UTATMatchQuestDescription::GetAssetRegistryTags(FAssetRegistryTagsContext context) const
{
   Super::GetAssetRegistryTags(context);

   // Don't bother putting in the cook for now
   if (!context.IsCooking() && !Map.IsNull())
   {
      context.AddTag(FAssetRegistryTag(NAME_Map, Map.GetLongPackageName(), FAssetRegistryTag::TT_Alphabetical));
   }
}


TArray<TSoftObjectPtr<UTATMatchQuestDescription>> UTATMatchQuestDescription::FindForMap(const TSoftObjectPtr<UWorld>& map)
{
   TRACE_CPUPROFILER_EVENT_SCOPE(UTATMatchQuestDescription::FindForMap)
   const IAssetRegistry& assetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();

   FARFilter filter;
   filter.ClassPaths.Add(UTATMatchQuestDescription::StaticClass()->GetClassPathName());
   filter.bIncludeOnlyOnDiskAssets = true;
   filter.TagsAndValues.Add(NAME_Map, map.GetLongPackageName());

   TArray<TSoftObjectPtr<UTATMatchQuestDescription>> result;
   assetRegistry.EnumerateAssets(filter, [&result](const FAssetData& assetData)
      {
         result.Emplace(assetData.GetSoftObjectPath());
         return true;
      });

   return result;
}

EDataValidationResult UTATMatchQuestDescription::IsDataValid(FDataValidationContext& context) const
{
   Super::IsDataValid(context);

   if (Map.IsNull())
   {
      context.AddWarning(INVTEXT("No Map specified, so unable to check against it (Let us know if there are valid reasons not to have one)"));
   }

   auto reportError = [&context](const FText& message) { context.AddWarning(message); };
   
   if (context.GetValidationUsecase() != EDataValidationUsecase::Commandlet)
   {
      // If the map happens to already be loaded, check combinations against that
      // This seems to work for level instances in PIE
      if (const UWorld* loadedMap = Map.Get())
      {
         TSet<FName> comboNames = CombinationScrape::GetLockCombinationNamesInWorld(loadedMap);
         CheckLockCombinations(comboNames, reportError);
      }
   }

   MatchQuestHelpers::ValidateParamSources(FormatParamSources, reportError);

   for(int choiceIndex = 0; choiceIndex < Choices.Num(); choiceIndex++)
   {
      const FTATQuestChoice& choice = Choices[choiceIndex];
      if(choice.Options.Num() < choice.NumToChoose.Max)
      {
         context.AddError(FText::Format(INVTEXT("Choice '{0}' (index {1}) has fewer options than it is trying to choose ({2} < {3})"), FText::FromString(choice.DebugName), choiceIndex,
            choice.Options.Num(), choice.NumToChoose.Max));
      }

      for(int optionIndex = 0; optionIndex < choice.Options.Num(); optionIndex++)
      {
         const FTATQuestChoiceOption& option = choice.Options[optionIndex];

         if(option.SceneVariant)
         {
            for(int otherOptionIndex = optionIndex+1; otherOptionIndex < choice.Options.Num(); otherOptionIndex++)
            {
               const FTATQuestChoiceOption& otherOption = choice.Options[otherOptionIndex];
               if(option.SceneVariant == otherOption.SceneVariant)
               {
                  context.AddError(FText::Format(INVTEXT("Choice '{0}' (index {1}) has two options with scene variant {2} at index {3} and {4}. Currently it is assumed that a variant implies a matching option would be taken"), FText::FromString(choice.DebugName), choiceIndex,
                     FText::FromString(option.SceneVariant.GetName()), optionIndex, otherOptionIndex));
               }
            }
         }

         MatchQuestHelpers::ValidateParamSources(FormatParamSources, [&context, choiceIndex, optionIndex](const FText& message)
         {
            context.AddWarning(FText::Format(INVTEXT("Choice '{0}' (index {1}): {2}"), choiceIndex, optionIndex, message));
         });
      }
   }

   // TODO: validate overrides against the scene sets they override (ideally extracting helper from scene set's validation code)

   return context.GetIssues().Num() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

void UTATMatchQuestDescription::CheckLockCombinations(const TSet<FName>& validLockCombinations, TFunctionRef<void(const FText&)> reportError) const
{
   for (const TPair<FString, FTATLockCombinationNameRef>& entry : LockCombinations)
   {
      if (!validLockCombinations.Contains(entry.Value.Name))
      {
         reportError(FText::Format(INVTEXT("Lock combination name '{0}' not found in level"),
            FText::FromName(entry.Value.Name)));
      }
   }

   for (int choiceIndex = 0; choiceIndex < Choices.Num(); choiceIndex++)
   {
      const FTATQuestChoice& choice = Choices[choiceIndex];
      for (int optionIndex = 0; optionIndex < choice.Options.Num(); optionIndex++)
      {
         const FTATQuestChoiceOption& option = choice.Options[optionIndex];
         for (const TPair<FString, FTATLockCombinationNameRef>& entry : option.LockCombinations)
         {
            if (!validLockCombinations.Contains(entry.Value.Name))
            {
               reportError(FText::Format(INVTEXT("Lock combination name '{0}' not found in level. [Choice '{1}' (index {2}), Option {3}]"),
                  FText::FromName(entry.Value.Name), FText::FromString(choice.DebugName), choiceIndex, optionIndex));
            }
         }
      }
   }
}

FGameplayTagContainer UTATMatchQuestDescription::CollectChoiceTags() const
{
    FGameplayTagContainer result;

    for (const FTATQuestChoice& choice : Choices)
    {
       for (const FTATQuestChoiceOption& option : choice.Options)
       {
          result.AppendTags(option.AddedChoiceTags);
       }
    }

    return result;
}
#endif

