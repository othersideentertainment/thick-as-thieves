// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Character/TATCharacterMetadata.h"
#include "CharacterCustomization/TATCharacterOutfits.h"
#include "Developer/TATOutfitSettings.h"
#include "Developer/TATProjectSettings.h"
#include "SaveGame/TATCharacterSaveId.h"
#include "Developer/TATToolSettings.h"
#include "Developer/TATAbilitySettings.h"
#include "SaveGame/TATCharacterDataContext.h"
#include "SaveGame/TATCharacterSaveId.h"
#include "SaveGame/TATSaveGame.h"
#include "Tools/TATToolTypes.h"
#include "Abilities/TATAbilityLoadoutTypes.h"
#include "Upgrades/TATCharacterUpgradeUtils.h"

// ue
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCharacterMetadata)

DEFINE_LOG_CATEGORY_STATIC(LogTATCharacterMetadata, Log, All);

bool FTATCharacterAvailableLoadoutEntry::MeetsUpgradeRequirement(const FTATCharacterDataContext& context) const
{
   return !UpgradeTag.IsValid() || UTATCharacterUpgradeUtils::GetCharacterUpgradeLevelForTag(context, UpgradeTag) >= UpgradeLevel;
}

const FTATCharacterAvailableLoadoutEntry* FTATCharacterLoadoutConfig::FindAvailableLoadoutEntryByTag(FGameplayTag loadoutTag) const
{
   return AvailableLoadout.FindByPredicate([loadoutTag](const FTATCharacterAvailableLoadoutEntry& entry)
   {
      return entry.LoadoutTag == loadoutTag;
   });
}

const FTATCharacterLoadoutBlock* FTATCharacterLoadoutConfig::FindLoadoutBlock(FGameplayTag category) const
{
   for (const FTATCharacterLoadoutBlock& block : LoadoutSlotBlocks)
   {
      if (block.SlotCategory == category)
      {
         return &block;
      }
   }
   return nullptr;
}

bool FTATCharacterLoadoutConfig::IsValidLoadout(const FTATCharacterDataContext& context, const FTATCharacterLoadout& loadout) const
{
   //NB. If you change the semantics for what a valid loadout is, you may also need to update FTATCharacterLoadoutConfig::IsDataValid to keep data validation in sync

   // Empty is fine, it'll get populated when needed
   if (loadout.Entries.Num() == 0)
   {
      return true;
   }

   TArray<FTATCharacterLoadoutCategorySpan, TInlineAllocator<16>> categorySpans;
   const int32 totalSlotCount = GetTotalSlotCountAndCategorySpans(context, categorySpans);

   // the slot count should match exactly
   if (loadout.Entries.Num() != totalSlotCount)
   {
      return false;
   }

   for (int32 i = 0; i < loadout.Entries.Num(); i++)
   {
      const FGameplayTag loadoutTag = loadout.Entries[i].LoadoutTag;

      // empty slots are fine
      if (!loadoutTag.IsValid())
      {
         continue;
      }

      // make sure the entry is in the available loadout array and that the character still meets the upgrade requirement
      const FTATCharacterAvailableLoadoutEntry* availableEntry = FindAvailableLoadoutEntryByTag(loadoutTag);
      if (availableEntry == nullptr || !availableEntry->MeetsUpgradeRequirement(context))
      {
         return false;
      }

      // make sure the entry's category is valid for the slot it's in
      if (!IsCategoryValidForSlot(categorySpans, UTATCharacterMetadataFunctionLibrary::GetLoadoutItemCategory(loadout.Type, loadoutTag), i))
      {
         return false;
      }
   }

   return true;
}

int32 FTATCharacterLoadoutConfig::GetLoadoutTotalSlotCount(const FTATCharacterDataContext& context) const
{
   int32 totalSlotCount = 0;
   for (const FTATCharacterLoadoutBlock& block : LoadoutSlotBlocks)
   {
      totalSlotCount += GetLoadoutSlotCountInCategory(context, block.SlotCategory);
   }
   return totalSlotCount;
}

int32 FTATCharacterLoadoutConfig::GetLoadoutSlotCountInCategory(const FTATCharacterDataContext& context, FGameplayTag loadoutCategory) const
{
   if (const FTATCharacterLoadoutBlock* block = FindLoadoutBlock(loadoutCategory))
   {
      int32 bonusFromUpgrades = 0;
      if (block->SlotCountUpgradeTag.IsValid())
      {
         constexpr int32 fallbackValue = -1;
         const int32 curveIndex = context.GetCharacterDataChecked().GetUpgradeValue(block->SlotCountUpgradeTag, fallbackValue);
         if (curveIndex != fallbackValue)
         {
            static const FString contextString = TEXT("FTATCharacterLoadoutConfig::GetLoadoutSlotCount");
            bonusFromUpgrades = block->SlotCountUpgradeCurve.AsInteger(static_cast<float>(curveIndex), &contextString);
         }
      }
      return FMath::Max(0, block->SlotCountBase) + FMath::Max(0, bonusFromUpgrades);
   }
   return 0;
}

// static
bool FTATCharacterLoadoutConfig::IsCategoryValidForSlot(TConstArrayView<FTATCharacterLoadoutCategorySpan> categorySpans, FGameplayTag category, int32 loadoutIndex)
{
   for (const FTATCharacterLoadoutCategorySpan& info : categorySpans)
   {
      if (info.ContainsIndex(loadoutIndex) && category == info.Category)
      {
         return true;
      }
   }
   return false;
}

#if WITH_EDITOR
void FTATCharacterLoadoutConfig::IsDataValid(FDataValidationContext& context, const FText& errorPrefix) const
{
   // Validate loadout slot blocks
   if (LoadoutSlotBlocks.Num() == 0)
   {
      context.AddError(FText::FormatOrdered(FTextFormat::FromString(TEXT("{0}: character has no loadout slot blocks")), errorPrefix));
   }
   else
   {
      for (int32 i = 0; i < LoadoutSlotBlocks.Num(); i++)
      {
         const FTATCharacterLoadoutBlock& block = LoadoutSlotBlocks[i];
         if (!block.SlotCategory.IsValid())
         {
            context.AddError(FText::FormatOrdered(FTextFormat::FromString(TEXT("{0}: loadout block at index {1} does not have a valid category tag")),
               errorPrefix, i));
         }
      }
   }

   // Validate the available loadout
   for (int32 i = 0; i < AvailableLoadout.Num(); i++)
   {
      const FTATCharacterAvailableLoadoutEntry& entry = AvailableLoadout[i];
      if (!entry.LoadoutTag.IsValid())
      {
         context.AddError(FText::FormatOrdered(FTextFormat::FromString(TEXT("{0}: available loadout entry {1} has an invalid loadout tag")),
            errorPrefix, i));
      }
      if (entry.UpgradeTag.IsValid() && entry.UpgradeLevel <= 0)
      {
         context.AddError(FText::FormatOrdered(FTextFormat::FromString(TEXT("{0}: available loadout entry {1} has an invalid upgrade level ({2})")),
            errorPrefix, i, entry.UpgradeLevel));
      }
   }

   // Make sure all tags in the default loadout are also in the available loadout
   for (int32 i = 0; i < DefaultLoadout.Num(); i++)
   {
      const FTATCharacterLoadoutEntry& entry = DefaultLoadout[i];
      if (FindAvailableLoadoutEntryByTag(entry.LoadoutTag) == nullptr)
      {
         context.AddError(FText::FormatOrdered(FTextFormat::FromString(TEXT("{0}: default loadout entry {1} (at index {2}) is not in the available loadout")),
            errorPrefix, FText::FromString(entry.LoadoutTag.ToString()), i));
      }
   }
}
#endif // WITH_EDITOR

#if WITH_EDITOR
void FTATCharacterMetadata::IsDataValid(FDataValidationContext& context, const FText& errorPrefix) const
{
   if (LocalizedName.IsEmpty())
   {
      context.AddError(FText::Format(INVTEXT("{0}: empty localized name"), errorPrefix));
   }

   //TODO: add data validation for ability loadout (once implemented)
   //AbilityLoadoutConfig.IsDataValid(context, FText::Format(INVTEXT("{0}: ability loadout config"), errorPrefix));

   ToolLoadoutConfig.IsDataValid(context, FText::Format(INVTEXT("{0}: tool loadout config"), errorPrefix));
   OutfitLoadoutConfig.IsDataValid(context, FText::Format(INVTEXT("{0}: outfit loadout config"), errorPrefix));

   for (const auto& pair : IntrinsicUpgrades)
   {
      if (pair.Value < 1)
      {
         context.AddError(FText::Format(INVTEXT("{0}: intrinsic upgrade {1} has a value of {2}, but upgrade values must be greater than or equal to 1"),
            errorPrefix, FText::FromString(pair.Key.ToString()), pair.Value));
      }
   }
}
#endif // WITH_EDITOR

#if WITH_EDITOR
EDataValidationResult UTATCharactersMetadata::IsDataValid(FDataValidationContext& context) const
{
   for (const auto& pair : Characters)
   {
      const ETATCharacter characterType = pair.Key;
      const FText errorPrefix = StaticEnum<ETATCharacter>()->GetDisplayNameTextByValue(static_cast<int64>(characterType));
      pair.Value.IsDataValid(context, errorPrefix);
   }

   return context.GetNumErrors() + context.GetNumWarnings() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif // WITH_EDITOR

const FTATCharacterMetadata& UTATCharactersMetadata::GetCharacterMetadata(ETATCharacter character) const
{
   if (Characters.Contains(character))
   {
      return Characters[character];
   }
   static FTATCharacterMetadata sEmptyMetadata;
   return sEmptyMetadata;
}

bool UTATCharactersMetadata::HasCharacter(ETATCharacter character) const
{
   return Characters.Contains(character);
}

TArray<ETATCharacter> UTATCharactersMetadata::GetAvailableCharacters() const
{
   TArray<ETATCharacter> availableCharacters;
   Characters.GenerateKeyArray(availableCharacters);
   return availableCharacters;
}

FTATCharacterMetadataHandle UTATCharactersMetadata::GetCharacterMetadataHandle(ETATCharacter character) const
{
   return FTATCharacterMetadataHandle(this, character);
}

bool UTATCharactersMetadata::GetCharacterNameAndDescription(ETATCharacter character, FText& name, FText& className, FText& nickname, FText& description) const
{
   if (const FTATCharacterMetadata* meta = Characters.Find(character))
   {
      name = meta->LocalizedName;
      className = meta->LocalizedClassName;
      nickname = meta->LocalizedNickname;
      description = meta->LocalizedDescription;
      return true;
   }
   name = FText::GetEmpty();
   className = FText::GetEmpty();
   nickname = FText::GetEmpty();
   description = FText::GetEmpty();
   return false;
}

bool UTATCharactersMetadata::GetCharacterTextures(ETATCharacter character, TSoftObjectPtr<UTexture2D>& symbolTexture, TSoftObjectPtr<UTexture2D>& characterSelectTexture, TSoftObjectPtr<UTexture2D>& characterFaceTexture) const
{
   if (const FTATCharacterMetadata* meta = Characters.Find(character))
   {
      symbolTexture = meta->SymbolTexture;
      characterSelectTexture = meta->CharacterSelectTexture;
      characterFaceTexture = meta->CharacterFaceTexture;
      return true;
   }
   symbolTexture.Reset();
   characterSelectTexture.Reset();
   characterFaceTexture.Reset();
   return false;
}

const FTATCharacterMetadata& FTATCharacterMetadataHandle::Get() const
{
   if (_asset)
   {
      return _asset->GetCharacterMetadata(_character);
   }

   static const FTATCharacterMetadata sEmptyMetadata;
   return sEmptyMetadata;
}

// static
UTATCharactersMetadata* UTATCharacterMetadataFunctionLibrary::GetCharactersMetadataAsset()
{
   return UTATProjectSettings::Get().DefaultCharacterMetadata.LoadSynchronous();
}

// static
const FTATCharacterMetadata* UTATCharacterMetadataFunctionLibrary::FindCharacterMetadataForCharacter(const FTATCharacterSaveId& characterSaveId)
{
   if (const UTATCharactersMetadata* metadata = GetCharactersMetadataAsset())
   {
      return metadata->Characters.Find(characterSaveId.Character);
   }
   return nullptr;
}

// static
const FTATCharacterLoadoutConfig* UTATCharacterMetadataFunctionLibrary::FindCharacterLoadoutConfigForCharacter(const FTATCharacterSaveId& characterSaveId, ETATLoadoutType loadoutType)
{
   if (const FTATCharacterMetadata* metadata = FindCharacterMetadataForCharacter(characterSaveId))
   {
      return metadata->GetLoadoutConfig(loadoutType);
   }
   return nullptr;
}

// static
FGameplayTag UTATCharacterMetadataFunctionLibrary::GetLoadoutItemCategory(ETATLoadoutType loadoutType, FGameplayTag loadoutTag)
{
   if (loadoutType == ETATLoadoutType::Tool)
   {
      if (const FTATGearMetadataTableRow* gearMetadata = UTATToolSettings::Get().FindGearMetadata(loadoutTag))
      {
         return gearMetadata->ToolLoadoutSlotCategory;
      }
   }
   if (loadoutType == ETATLoadoutType::Ability)
   {
      if (const FTATAbilityLoadoutMetadataTableRow* abilityMetadata = UTATAbilitySettings::Get().FindAbilityMetadata(loadoutTag))
      {
         return abilityMetadata->LoadoutSlotCategory;
      }
   }
   if (loadoutType == ETATLoadoutType::Outfit)
   {
      if (const FTATOutfitsMetadataTableRow* outfitMetadata = UTATOutfitSettings::Get().FindOutfitMetadata(loadoutTag))
      {
         return outfitMetadata->OutfitLoadoutSlotCategory;
      }
   }
   return FGameplayTag::EmptyTag;
}

// static
bool UTATCharacterMetadataFunctionLibrary::GetAllLoadoutCategories(ETATLoadoutType loadoutType, TArray<FGameplayTag>& loadoutCategories)
{
   loadoutCategories.Reset();
   if (loadoutType == ETATLoadoutType::Tool)
   {
      UTATToolSettings::Get().ForEachGearMetadataRow([&loadoutCategories](const FTATGearMetadataTableRow& row)
      {
         loadoutCategories.AddUnique(row.ToolLoadoutSlotCategory);
      });
   }
   else if (loadoutType == ETATLoadoutType::Ability)
   {
      UTATAbilitySettings::Get().ForEachAbilityMetadataRow([&loadoutCategories](const FTATAbilityLoadoutMetadataTableRow& row)
      {
         loadoutCategories.AddUnique(row.LoadoutSlotCategory);
      });
   }
   else
   {
      //TODO: Implement non-gear loadout types if/when needed
      UE_LOG(LogTATCharacterMetadata, Error, TEXT("UTATCharacterMetadataFunctionLibrary::GetAllLoadoutCategories only supports gear and ability loadout types at this time"));
   }
   return loadoutCategories.Num() > 0;
}

// static
bool UTATCharacterMetadataFunctionLibrary::GetAllLoadoutCategoriesForCharacter(const FTATCharacterDataContext& context, ETATLoadoutType loadoutType, TArray<FTATCharacterLoadoutCategorySpan>& loadoutCategories)
{
   loadoutCategories.Reset();
   if (const FTATCharacterLoadoutConfig* loadoutConfig = FindCharacterLoadoutConfigForCharacter(context.SaveId, loadoutType))
   {
      loadoutConfig->GetTotalSlotCountAndCategorySpans(context, loadoutCategories);
   }
   return loadoutCategories.Num() > 0;
}

// static
bool UTATCharacterMetadataFunctionLibrary::IsValidLoadoutForCharacter(const FTATCharacterDataContext& context, ETATLoadoutType loadoutType, const FTATCharacterLoadout& loadout)
{
   if (const FTATCharacterLoadoutConfig* loadoutConfig = FindCharacterLoadoutConfigForCharacter(context.SaveId, loadoutType))
   {
      return loadoutConfig->IsValidLoadout(context, loadout);
   }

   // If we don't have any valid metadata, just assume anything is valid
   return true;
}

// static
void UTATCharacterMetadataFunctionLibrary::MakeCharacterLoadoutValid(const FTATCharacterDataContext& context, FTATCharacterLoadout& inOutLoadout)
{
   const FTATCharacterLoadoutConfig* loadoutConfig = FindCharacterLoadoutConfigForCharacter(context.SaveId, inOutLoadout.Type);
   if (loadoutConfig == nullptr)
   {
      return;
   }

   // Collect all valid categories
   TArray<FTATCharacterLoadoutCategorySpan, TInlineAllocator<8>> loadoutCategories;
   const int32 resultTotalSlotCount = loadoutConfig->GetTotalSlotCountAndCategorySpans(context, loadoutCategories);

   // Slot count of zero means we're not allowed anything in the loadout, so just empty it
   if (resultTotalSlotCount <= 0)
   {
      inOutLoadout.Entries.Reset();
      return;
   }

   // Collect all existing valid entries
   struct FOrigLoadoutEntry
   {
      FTATCharacterLoadoutEntry Entry;
      FGameplayTag Category;
      int32 Index = INDEX_NONE;
   };
   TArray<FOrigLoadoutEntry, TInlineAllocator<16>> origValidEntries;
   for (int32 i = 0; i < inOutLoadout.Entries.Num(); i++)
   {
      if (!inOutLoadout.Entries[i].LoadoutTag.IsValid())
      {
         continue;
      }

      // Only include entries that are in AvailableEntries and make sure the character still has the upgrade requirement for it.
      const FTATCharacterAvailableLoadoutEntry* availEntry = loadoutConfig->FindAvailableLoadoutEntryByTag(inOutLoadout.Entries[i].LoadoutTag);
      if (availEntry != nullptr && availEntry->MeetsUpgradeRequirement(context))
      {
         FOrigLoadoutEntry origEntry{};
         origEntry.Entry = inOutLoadout.Entries[i];
         // look up and store the category here to avoid repeated lookups later
         origEntry.Category = GetLoadoutItemCategory(inOutLoadout.Type, inOutLoadout.Entries[i].LoadoutTag);
         origEntry.Index = i;
         origValidEntries.Add(origEntry);
      }
   }

   // Now rebuild the loadout
   inOutLoadout.Entries.Reset();
   inOutLoadout.Entries.SetNum(resultTotalSlotCount);

   // First, assign any existing valid loadout entries to their original index if possible
   for (FOrigLoadoutEntry& origEntry : origValidEntries)
   {
      if (!inOutLoadout.Entries.IsValidIndex(origEntry.Index))
      {
         continue;
      }
      const bool slotAvailable = !inOutLoadout.Entries[origEntry.Index].LoadoutTag.IsValid();
      if (slotAvailable && FTATCharacterLoadoutConfig::IsCategoryValidForSlot(loadoutCategories, origEntry.Category, origEntry.Index))
      {
         inOutLoadout.Entries[origEntry.Index] = origEntry.Entry;
         origEntry.Index = INDEX_NONE; // mark as assigned
      }
   }

   // Next, if we have loadout entries that couldn't be assigned to their original slot (eg. the requirements changed),
   // find another valid slot to assign them to if possible
   for (FOrigLoadoutEntry& origEntry : origValidEntries)
   {
      if (origEntry.Index == INDEX_NONE)
      {
         // already assigned
         continue;
      }

      int32 newValidIndex = INDEX_NONE;
      for (int32 i = 0; i < inOutLoadout.Entries.Num(); i++)
      {
         const bool slotAvailable = !inOutLoadout.Entries[i].LoadoutTag.IsValid();
         if (slotAvailable && FTATCharacterLoadoutConfig::IsCategoryValidForSlot(loadoutCategories, origEntry.Category, i))
         {
            newValidIndex = i;
            break;
         }
      }

      if (newValidIndex != INDEX_NONE)
      {
         inOutLoadout.Entries[newValidIndex] = origEntry.Entry;
         origEntry.Index = INDEX_NONE; // mark as assigned
      }
   }

   check(inOutLoadout.NumEntries() == resultTotalSlotCount);
}

// static
bool UTATCharacterMetadataFunctionLibrary::GetMaxNumLoadoutSlotsForCharacter(const FTATCharacterDataContext& context, ETATLoadoutType loadoutType, int32& maxSlotCount)
{
   if (const FTATCharacterLoadoutConfig* loadoutConfig = FindCharacterLoadoutConfigForCharacter(context.SaveId, loadoutType))
   {
      maxSlotCount = loadoutConfig->GetLoadoutTotalSlotCount(context);
      return true;
   }
   maxSlotCount = 0;
   return false;
}

// static
bool UTATCharacterMetadataFunctionLibrary::GetNumLoadoutSlotsInCategoryForCharacter(const FTATCharacterDataContext& context, ETATLoadoutType loadoutType, FGameplayTag loadoutCategory, int32& slotCount)
{
   if (const FTATCharacterLoadoutConfig* loadoutConfig = FindCharacterLoadoutConfigForCharacter(context.SaveId, loadoutType))
   {
      slotCount = loadoutConfig->GetLoadoutSlotCountInCategory(context, loadoutCategory);
      return true;
   }
   slotCount = 0;
   return false;
}

// static
bool UTATCharacterMetadataFunctionLibrary::GetLoadoutCategoryForSlotIndexAndCharacter(const FTATCharacterDataContext& context, ETATLoadoutType loadoutType, int32 slotIndex, FGameplayTag& loadoutCategory)
{
   loadoutCategory = FGameplayTag::EmptyTag;
   
   if (slotIndex < 0)
   {
      return false;
   }

   const FTATCharacterLoadoutConfig* loadoutConfig = FindCharacterLoadoutConfigForCharacter(context.SaveId, loadoutType);
   if (loadoutConfig == nullptr)
   {
      return false;
   }

   TArray<FTATCharacterLoadoutCategorySpan, TInlineAllocator<16>> categorySpans;
   const int32 totalSlotCount = loadoutConfig->GetTotalSlotCountAndCategorySpans(context, categorySpans);
   if (slotIndex >= totalSlotCount)
   {
      return false;
   }

   for (const FTATCharacterLoadoutCategorySpan& categorySpan : categorySpans)
   {
      if (slotIndex >= categorySpan.StartIndex && slotIndex < categorySpan.StartIndex + categorySpan.SlotCount)
      {
         loadoutCategory = categorySpan.Category;
         return true;
      }
   }

   return false;
}

// static
bool UTATCharacterMetadataFunctionLibrary::GetAvailableLoadoutInCategoryForCharacter(const FTATCharacterDataContext& context, ETATLoadoutType loadoutType, FGameplayTag loadoutCategory, TArray<FTATCharacterAvailableLoadoutEntry>& availableLoadout)
{
   availableLoadout.Reset();

   if (!loadoutCategory.IsValid())
   {
      return false;
   }

   const FTATCharacterLoadoutConfig* loadoutConfig = FindCharacterLoadoutConfigForCharacter(context.SaveId, loadoutType);
   if (loadoutConfig == nullptr)
   {
      return false;
   }

   const bool categoryAvailable = loadoutConfig->LoadoutSlotBlocks.ContainsByPredicate([loadoutCategory](const FTATCharacterLoadoutBlock& slotBlock)
   {
      return slotBlock.SlotCategory == loadoutCategory;
   });
   if (!categoryAvailable)
   {
      return false;
   }

   for (const FTATCharacterAvailableLoadoutEntry& entry : loadoutConfig->AvailableLoadout)
   {
      if (GetLoadoutItemCategory(loadoutType, entry.LoadoutTag) == loadoutCategory && entry.MeetsUpgradeRequirement(context))
      {
         availableLoadout.Add(entry);
      }
   }

   return true;
}

// static
bool UTATCharacterMetadataFunctionLibrary::GetAvailableLoadoutForCharacter(const FTATCharacterDataContext& context, ETATLoadoutType loadoutType, TArray<FTATCharacterAvailableLoadoutEntry>& availableLoadout)
{
   if (const FTATCharacterLoadoutConfig* loadoutConfig = FindCharacterLoadoutConfigForCharacter(context.SaveId, loadoutType))
   {
      availableLoadout = loadoutConfig->AvailableLoadout;
      return true;
   }
   availableLoadout.Reset();
   return false;
}

// static
bool UTATCharacterMetadataFunctionLibrary::GetAvailableLoadoutTagsForCharacter(const FTATCharacterDataContext& context, ETATLoadoutType loadoutType, TArray<FGameplayTag>& availableLoadoutTags, bool filterOutTagsWithUnmetReqs)
{
   if (const FTATCharacterLoadoutConfig* loadoutConfig = FindCharacterLoadoutConfigForCharacter(context.SaveId, loadoutType))
   {
      availableLoadoutTags.Reset();
      for (const FTATCharacterAvailableLoadoutEntry& entry : loadoutConfig->AvailableLoadout)
      {
         if (!filterOutTagsWithUnmetReqs || entry.MeetsUpgradeRequirement(context))
         {
            availableLoadoutTags.Add(entry.LoadoutTag);
         }
      }
      return true;
   }
   availableLoadoutTags.Reset();
   return false;
}

// static
bool UTATCharacterMetadataFunctionLibrary::GetDefaultLoadoutForCharacter(const FTATCharacterDataContext& context, ETATLoadoutType loadoutType, FTATCharacterLoadout& defaultLoadout)
{
   const FTATCharacterLoadoutConfig* loadoutConfig = FindCharacterLoadoutConfigForCharacter(context.SaveId, loadoutType);
   if (loadoutConfig == nullptr)
   {
      defaultLoadout = FTATCharacterLoadout{};
      return false;
   }

   defaultLoadout.Type = loadoutType;

   TArray<FTATCharacterLoadoutCategorySpan, TInlineAllocator<16>> categorySpans;
   const int32 totalNumLoadoutSlots = loadoutConfig->GetTotalSlotCountAndCategorySpans(context, categorySpans);

   defaultLoadout.Entries.SetNum(totalNumLoadoutSlots);
   if (totalNumLoadoutSlots == 0)
   {
      return true;
   }

   for (const FTATCharacterLoadoutEntry& defaultEntry : loadoutConfig->DefaultLoadout)
   {
      const FTATCharacterAvailableLoadoutEntry* availableLoadoutEntry = loadoutConfig->FindAvailableLoadoutEntryByTag(defaultEntry.LoadoutTag);
      if (availableLoadoutEntry == nullptr)
      {
         // No available loadout entry. Complain about it but assume it's supposed to be in the defaults since it was placed there explicitly.
         // (this should also cause data validation to fail)
         UE_LOG(LogTATCharacterMetadata, Error, TEXT("Character %s has '%s' in the %s default loadout, but the available loadout does not have an entry for that tag."),
            *StaticEnum<ETATLoadoutType>()->GetNameStringByValue((int64)context.SaveId.Character),
            *defaultEntry.LoadoutTag.ToString(),
            *StaticEnum<ETATLoadoutType>()->GetNameStringByValue((int64)loadoutType));
      }

      // Check upgrade requirements
      if (availableLoadoutEntry != nullptr && !availableLoadoutEntry->MeetsUpgradeRequirement(context))
      {
         continue;
      }

      // Place the entry in the first valid slot we find
      int32 resultIndex = INDEX_NONE;
      for (int32 i = 0; i < defaultLoadout.Entries.Num(); i++)
      {
         if (defaultLoadout.Entries[i].LoadoutTag.IsValid())
         {
            // something is already assigned to this index
            continue;
         }
         if (FTATCharacterLoadoutConfig::IsCategoryValidForSlot(categorySpans, GetLoadoutItemCategory(loadoutType, defaultEntry.LoadoutTag), i))
         {
            resultIndex = i;
            break;
         }
      }

      if (resultIndex != INDEX_NONE)
      {
         defaultLoadout.Entries[resultIndex] = defaultEntry;
      }
   }

   return true;
}

// static
bool UTATCharacterMetadataFunctionLibrary::GetLoadoutTagsFeaturedOnCharacterSelectScreen(ETATCharacter character, ETATLoadoutType loadoutType, TArray<FGameplayTag>& characterSelectScreenTags)
{
   const FTATCharacterLoadoutConfig* loadoutConfig = FindCharacterLoadoutConfigForCharacter(FTATCharacterSaveId::FromCharacterType(character), loadoutType);
   if (loadoutConfig == nullptr)
   {
      characterSelectScreenTags.Reset();
      return false;
   }
   characterSelectScreenTags = loadoutConfig->FeaturedOnCharacterSelectScreen;
   return true;
}

FTATCharacterMetadataHandle UTATCharacterMetadataFunctionLibrary::CreateCharacterMetadataHandle(ETATCharacter character, const UObject* worldContext)
{
   // Not using worldContext, but I want to make it available
   return FTATCharacterMetadataHandle(GetCharactersMetadataAsset(), character);
}
