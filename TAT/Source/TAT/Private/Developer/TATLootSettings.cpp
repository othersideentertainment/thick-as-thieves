// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Developer/TATLootSettings.h"

// tat
#include "Loot/TATLootSubsystem.h"
#include "Loot/TATLootTypes.h"


// ue
#include "Engine/DataTable.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLootSettings)

DEFINE_LOG_CATEGORY_STATIC(LogTATLootSettings, Log, All);


UTATLootSettings::UTATLootSettings()
   : Super()
{
}

const FTATLootInfo* UTATLootSettings::FindLootInfo(const UObject* contextObject, const FTATLootIdentifier& lootIdentifier) const
{
   if (const UTATLootSubsystem* lootSubsystem = UTATLootSubsystem::Get(contextObject))
   {
      return lootSubsystem->FindLootInfo(lootIdentifier);
   }
   return nullptr;
}

const FTATLootInfo* UTATLootSettings::GetLootInfo(const UObject* contextObject, const FTATLootIdentifier& lootIdentifier) const
{
   if (!lootIdentifier.IsValid())
   {
      UE_LOG(LogTATLootSettings, Warning, TEXT("GetLootInfo() called with invalid lootIdentifier! Could not return loot info"));
      return nullptr;
   }

   const UTATLootSubsystem* lootSubsystem = UTATLootSubsystem::Get(contextObject);
   if (lootSubsystem == nullptr)
   {
      UE_LOG(LogTATLootSettings, Warning, TEXT("GetLootInfo() failed to find the loot subsystem! Could not return loot info"));
      return nullptr;
   }

   const FTATLootInfo* lootInfo = lootSubsystem->FindLootInfo(lootIdentifier);
   if (lootInfo == nullptr)
   {
      UE_LOG(LogTATLootSettings, Warning, TEXT("GetLootInfo() | Could not find loot with LootTag %s!"), *lootIdentifier.LootTag.ToString());
      return nullptr;
   }
   return lootInfo;
}

const FTATLootInfo* UTATLootSettings::GetLootInfo(const FDataTableRowHandle& lootRowHandle) const
{
   return lootRowHandle.GetRow<FTATLootInfo>(TEXT("GetLootInfoByRowHandle"));
}

const FTATLootInfo& UTATLootSettings::GetLootInfoChecked(const UObject* contextObject, const FTATLootIdentifier& lootIdentifier) const
{
   const UTATLootSubsystem* lootSubsystem = UTATLootSubsystem::Get(contextObject);
   check(lootSubsystem != nullptr);
   const FTATLootInfo* lootInfo = lootSubsystem->FindLootInfo(lootIdentifier);
   checkf(lootInfo != nullptr, TEXT("Invalid loot identifier '%s'"), *lootIdentifier.LootTag.ToString());
   return *lootInfo;
}

FGameplayTag UTATLootSettings::GetLootPickupGameplayCue(const FTATLootInfo& lootInfo) const
{
   // Check for loot-specific override
   if (lootInfo.PickupGameplayCueOverride.IsValid())
   {
      return lootInfo.PickupGameplayCueOverride;
   }

   // Select default audio event
   switch (lootInfo.LootType)
   {
   case ETATLootType::MinorLoot:
      return MinorLootPickupGameplayCue;
   case ETATLootType::MajorLoot:
      return MajorLootPickupGameplayCue;

   default:
      checkNoEntry();
      return FGameplayTag::EmptyTag;
   }
}

FGameplayTag UTATLootSettings::GetLootPickupGameplayEventTag(const FTATLootInfo& lootInfo) const
{
   switch (lootInfo.LootType)
   {
   case ETATLootType::MinorLoot:
      return MinorLootPickupGameplayEventTag;
   case ETATLootType::MajorLoot:
      return MajorLootPickupGameplayEventTag;

   default:
      checkNoEntry();
      return FGameplayTag::EmptyTag;
   }
}

TOptional<int32> UTATLootSettings::GetSlotSize(const UObject* contextObject, const FTATLootIdentifier& lootIdentifier) const
{
   if (const FTATLootInfo* lootInfo = FindLootInfo(contextObject, lootIdentifier))
   {
      return lootInfo->GetSlotSize();
   }
   return NullOpt;
}

int32 UTATLootSettings::GetSlotSizeCombined(const UObject* contextObject, TConstArrayView<FTATLootIdentifier> lootIds) const
{
   int32 combinedSlotSize = 0;
   for (const FTATLootIdentifier& lootId : lootIds)
   {
      if (const FTATLootInfo* lootInfo = FindLootInfo(contextObject, lootId))
      {
         combinedSlotSize += lootInfo->GetSlotSize();
      }
   }
   return combinedSlotSize;
}

int32 UTATLootSettings::GetSlotSizeCombined(const UObject* contextObject, TConstArrayView<FTATLootInstance> lootInstances) const
{
   int32 combinedSlotSize = 0;
   for (const FTATLootInstance& lootInstance : lootInstances)
   {
      if (const FTATLootInfo* lootInfo = FindLootInfo(contextObject, lootInstance.Identifier))
      {
         combinedSlotSize += lootInfo->GetSlotSize();
      }
   }
   return combinedSlotSize;
}

int32 UTATLootSettings::GetSlotSizeCombined(const UObject* contextObject, TConstArrayView<FTATLootItemVariant> lootItems) const
{
   int32 combinedSlotSize = 0;
   for (const FTATLootItemVariant& lootItem : lootItems)
   {
      if (const FTATLootInfo* lootInfo = FindLootInfo(contextObject, lootItem.GetIdentifier()))
      {
         combinedSlotSize += lootInfo->GetSlotSize();
      }
   }
   return combinedSlotSize;
}

float UTATLootSettings::GetLootMultiplierForCurrentDifficulty(const UObject* contextObject) const
{
   const ETATDifficulty difficulty = TATDifficulty::GetDifficultyForMatch(contextObject->GetWorld());
   float multiplier = 1.f;
   if (const float* foundDifficultyMultiplier = DifficultyToLootMultiplier.Find(difficulty))
   {
      multiplier = *foundDifficultyMultiplier;
   }
   return multiplier;
}

int32 UTATLootSettings::GetLootValue(const UObject* contextObject, const FTATLootIdentifier& lootIdentifier, const bool shouldUseDifficultyMultiplier) const
{
   const FTATLootInfo* lootInfo = FindLootInfo(contextObject, lootIdentifier);
   float multiplier = 1.f;
   if (shouldUseDifficultyMultiplier && lootInfo != nullptr)
   {
      multiplier = GetLootMultiplierForCurrentDifficulty(contextObject);
   }
   return (lootInfo != nullptr) ? (lootInfo->Value * multiplier) : 0;
}

int32 UTATLootSettings::GetLootValue(const UObject* contextObject, const FTATLootInstance& lootInstance, const bool shouldUseDifficultyMultiplier) const
{
   return GetLootValue(contextObject, lootInstance.Identifier, shouldUseDifficultyMultiplier);
}

int32 UTATLootSettings::GetLootValue(const UObject* contextObject, TConstArrayView<FTATLootIdentifier> lootIdentifiers) const
{
   int32 totalValue = 0;
   for (const FTATLootIdentifier& lootId : lootIdentifiers)
   {
      totalValue += GetLootValue(contextObject, lootId, false);
   }
   const float multiplier = GetLootMultiplierForCurrentDifficulty(contextObject);
   return totalValue * multiplier;
}

int32 UTATLootSettings::GetLootValue(const UObject* contextObject, TConstArrayView<FTATLootInstance> lootInstances) const
{
   int32 totalValue = 0;
   for (const FTATLootInstance& lootInst : lootInstances)
   {
      totalValue += GetLootValue(contextObject, lootInst, false);
   }
   const float multiplier = GetLootMultiplierForCurrentDifficulty(contextObject);
   return totalValue * multiplier;
}

int32 UTATLootSettings::GetLootValue(const UObject* contextObject, const FTATLootContainer& lootContainer) const
{
   return GetLootValue(contextObject, lootContainer.GetItems()) + GetLootValue(contextObject, lootContainer.GetInstances());
}

bool UTATLootSettings::DoesAutoConvertToMoney(const UObject* contextObject, const FTATLootIdentifier& lootIdentifier) const
{
   if (const FTATLootInfo* lootInfo = FindLootInfo(contextObject, lootIdentifier))
   {
      return lootInfo->AutoConvertToMoney;
   }
   return false;
}

const UDataTable* UTATLootSettings::_GetLootDataTable(const UObject* contextObject) const
{
   const UTATLootSubsystem* lootSubsystem = UTATLootSubsystem::Get(contextObject);
   check(IsValid(lootSubsystem));
   return lootSubsystem->GetLootDataTable();
}
