// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/TATClueSet.h"

// tat
#include "Developer/TATCycleChecker.h"
#include "Variation/Clues/TATClueInfo.h"
#include "Variation/TATMapVariationSeedHelpers.h"

// ue
#include "Misc/DataValidation.h"
#include "StructUtils/StructView.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "UObject/AssetRegistryTagsContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATClueSet)

namespace ClueSetHelpers
{
   static const FName kClueSourceTag("ClueSource");
   static const FName kClueLocationTag("ClueLocation");
   static const FName KLookupTag("UseForLookup");
}

void UTATClueSet::AddRelevantClueViews(const FTATClueSetContext& context, TArray<FConstStructView>& result) const
{
   if(ParentClueSet)
   {
      ParentClueSet->AddRelevantClueViews(context, result);
   }
   
   result.Reserve(result.Num() + Clues.Num());
   for(const FTATClueSetEntry& clueEntry : Clues)
   {
      if(clueEntry.Enabled && clueEntry.Clue.IsValid())
      {
         result.Emplace(clueEntry.Clue);
      }
   }
}

UTATClueSet::FClueSetArray UTATClueSet::FindMatchingSets(FGameplayTag sourceTag, FGameplayTag locationTag)
{
   TRACE_CPUPROFILER_EVENT_SCOPE(UTATClueSet::FindMatchingSets)
   const IAssetRegistry& assetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();

   FARFilter filter;
   filter.ClassPaths.Add(UTATClueSet::StaticClass()->GetClassPathName());
   filter.bIncludeOnlyOnDiskAssets = true;

   // NOTE: The ARFilter for asset tags treats it as an "or", so can't use that. And since it indexes by tag presence,
   //       there isn't much value in doing any of them in the filter.
   // NOTE: The current assumption is that, for a given match, the number of clue sets being asked for may not merit
   //       building a lookup data structure. But that needs to be re-examined as number of clue sets grow.
   //       Or if doing as part of simulation.
   //       Current timing in editor is ~20-40us.
   FClueSetArray result;
   assetRegistry.EnumerateAssets(filter, [&result, sourceTag = sourceTag.ToString(), locationTag = locationTag.ToString()](const FAssetData& assetData)
   {
      if (assetData.TagsAndValues.FindTag(ClueSetHelpers::KLookupTag).Equals(TEXTVIEW("True")) &&
         assetData.TagsAndValues.FindTag(ClueSetHelpers::kClueSourceTag).Equals(sourceTag) &&
         assetData.TagsAndValues.FindTag(ClueSetHelpers::kClueLocationTag).Equals(locationTag))
      {
         result.Add(assetData.GetSoftObjectPath());
      }
      return true;
   });

   return result;
}

FSoftObjectPath UTATClueSet::FindRandomMatchingSet(FGameplayTag sourceTag, FGameplayTag locationTag, int32 seed)
{
   FClueSetArray matchingSets = FindMatchingSets(sourceTag, locationTag);
   if(matchingSets.Num() == 1)
   {
      return matchingSets[0];
   }
   else if(matchingSets.Num())
   {
      matchingSets.Sort([](const FSoftObjectPath& a, const FSoftObjectPath& b) { return a.GetAssetFName().Compare(b.GetAssetFName()) < 0; });
      FRandomStream randomStream(SeedHelpers::MakeSeedForName(sourceTag.GetTagName(), seed));
      return matchingSets[randomStream.RandHelper(matchingSets.Num())];
   }

   return FSoftObjectPath();
}

void UTATClueSet::GetAssetRegistryTags(FAssetRegistryTagsContext context) const
{
   Super::GetAssetRegistryTags(context);

   context.AddTag(FAssetRegistryTag(ClueSetHelpers::KLookupTag, UseForLookup ? TEXT("True") : TEXT("False"), FAssetRegistryTag::TT_Alphabetical));
   context.AddTag(FAssetRegistryTag(ClueSetHelpers::kClueSourceTag, ClueSource.ToString(), FAssetRegistryTag::TT_Alphabetical));
   context.AddTag(FAssetRegistryTag(ClueSetHelpers::kClueLocationTag, ClueLocation.ToString(), FAssetRegistryTag::TT_Alphabetical));
}

#if WITH_EDITOR
EDataValidationResult UTATClueSet::IsDataValid(class FDataValidationContext& context) const
{
   Super::IsDataValid(context);

   if(ParentClueSet)
   {
      TTATObjectCycleChecker<UTATClueSetBase> cycleChecker([](const UTATClueSetBase* clueSet, TFunctionRef<void(const UTATClueSetBase*)> visitor) { clueSet->VisitClueSetDependencies(visitor); });
      cycleChecker.DetectCycles(this, [&context](const TConstArrayView<const UTATClueSetBase*>& foundCycle)
         {
            TSharedRef<FTokenizedMessage> message = context.AddMessage(EMessageSeverity::Error, INVTEXT("Cycle in clue sets"));
            TATCycleChecker::AddCycleError(*message, foundCycle);
         });
   }

   if(UseForLookup)
   {
      if(!ClueSource.IsValid())
      {
         context.AddError(FText::FormatOrdered(INVTEXT("[{0}] No ClueSource tag for lookup-enabled ClueSet"),
            FText::FromString(GetName())));
      }
      if(!ClueLocation.IsValid())
      {
         context.AddError(FText::FormatOrdered(INVTEXT("[{0}] No ClueLocation tag for lookup-enabled ClueSet"),
            FText::FromString(GetName())));
      }
   }

   for(int i = 0; i < Clues.Num(); ++i)
   {
      const FInstancedStruct& clue = Clues[i].Clue;
      if(!clue.IsValid())
      {
         context.AddError(FText::FormatOrdered(INVTEXT("[{0}] No clue at index {1}"),
            FText::FromString(GetName()), FText::AsNumber(i)));
         continue;
      }

      clue.Get<FTATClueInfo>().Validate([&context, i, this](const FText& message)
      {
         context.AddError(FText::FormatOrdered(INVTEXT("[{0}] Error at clue index {1}: {2}"),
            FText::FromString(GetName()), FText::AsNumber(i), message));
      });
   }

   return context.GetIssues().Num() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

void UTATClueSet::VisitClueSetDependencies(TFunctionRef<void(const UTATClueSetBase*)> visitor) const
{
   if (ParentClueSet)
   {
      visitor(ParentClueSet);
   }
}

#endif

bool FTATClueSetEntry::SerializeFromMismatchedTag(const FPropertyTag& tag, FStructuredArchive::FSlot slot)
{
   if (tag.GetType().IsStruct(FInstancedStruct::StaticStruct()->GetFName()))
   {
      FInstancedStruct::StaticStruct()->SerializeItem(slot, &Clue, nullptr);
      return true;
   }
   
   return false;
}

bool FTATClueSetEntry::ImportTextItem(const TCHAR*& buffer, int32 portFlags, UObject* parent, FOutputDevice* errorText)
{
   // first do default behavior without customization
   if (const TCHAR* newBuffer = FTATClueSetEntry::StaticStruct()->ImportText(buffer, this, parent, portFlags, errorText, FString(), false))
   {
      buffer = newBuffer;
      return true;
   }

   // Try just importing the clue
   if(const TCHAR * clueBuffer = FInstancedStruct::StaticStruct()->ImportText(buffer, &Clue, parent, portFlags, errorText, FString(), true))
   {
      buffer = clueBuffer;
      return true;
   }

   return false;
}
