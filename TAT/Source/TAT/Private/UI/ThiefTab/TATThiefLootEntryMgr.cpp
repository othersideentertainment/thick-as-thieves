// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/ThiefTab/TATThiefLootEntryMgr.h"

// tat
#include "Developer/TATLootSettings.h"
#include "Loot/TATLootUtils.h"
#include "UI/ThiefTab/TATThiefLootEntryWidget.h"

// ue
#include "Components/ListView.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATThiefLootEntryMgr)
DEFINE_LOG_CATEGORY_STATIC(LogTATThiefLootEntryMgr, Log, All);

void FTATThiefLootEntryMgr::RemoveLootListEntry(UTATThiefLootEntryData* lootEntry)
{
   check(IsValid(lootEntry));
   
   // Remove element from appropriate set
   const FTATLootInfo& lootInfo = lootEntry->GetLootInfo();
   TSet<UTATThiefLootEntryData*>& lootListEntries = _GetLootEntrySetForType(lootInfo.LootType);
   ensure(lootListEntries.Remove(lootEntry) == 1);

   // Move back to object pool for subsequent use
   _ReturnLootEntryObjectToPool(lootEntry);
}

void FTATThiefLootEntryMgr::ClearLootListEntries()
{
   RemoveListEntriesOfType(ETATLootType::MinorLoot);
   RemoveListEntriesOfType(ETATLootType::MajorLoot);
}

void FTATThiefLootEntryMgr::_ConstructListEntry(const FTATLootIdentifier& lootIdentifier, UListView* listView, const FTATLootInstance* lootInstanceData)
{
   check(lootIdentifier.IsValid());
   check(IsValid(listView));
   check(lootInstanceData == nullptr || lootInstanceData->Identifier == lootIdentifier);

   // Track in appropriate set for subsequent reference
   TSet<UTATThiefLootEntryData*>& lootListEntries = _GetLootEntrySetForType(UTATLootUtils::GetLootType(listView, lootIdentifier));

   bool addNewEntry = false;

   // Can only stack non-instanced loot
   if (lootInstanceData == nullptr)
   {
      // Find and increment a stack of this loot type, if any
      bool bStackFound = false;
      for (UTATThiefLootEntryData* listEntry : lootListEntries)
      {
         if (listEntry->LootInstance.Identifier == lootIdentifier)
         {
            UE_LOG(LogTATThiefLootEntryMgr, Verbose, TEXT("Incrementing list entry for loot %s in list view %s"), *lootIdentifier.LootTag.ToString(), *listView->GetName());

            listEntry->Count += 1;
            bStackFound = true;
            break;
         }
      }

      // First entry of this loot type
      addNewEntry = !bStackFound;
   }
   else
   {
      addNewEntry = true;
   }

   if (addNewEntry)
   {
      UE_LOG(LogTATThiefLootEntryMgr, Verbose, TEXT("Constructing list entry for loot %s in list view %s"), *lootIdentifier.LootTag.ToString(), *listView->GetName());

      // Pick a matching loot entry data object from pool if present, creating one if not found
      UTATThiefLootEntryData* lootEntryData = _FindOrCreateLootEntryObject(lootIdentifier, listView, lootInstanceData);
      check(IsValid(lootEntryData));
      lootListEntries.Add(lootEntryData);

      // Add to list view. Slate will paint this asynchronously, so this entry can still receive Count updates in the above loop before it the widget is made
      listView->AddItem(lootEntryData);
   }
}

UTATThiefLootEntryData* FTATThiefLootEntryMgr::_FindOrCreateLootEntryObject(const FTATLootIdentifier& lootIdentifier, UObject* owner, const FTATLootInstance* lootInstanceData)
{
   check(lootIdentifier.IsValid());
   check(IsValid(owner));
   check(lootInstanceData == nullptr || lootInstanceData->Identifier == lootIdentifier);

   // Search for an existing entry in the object pool (but only if the loot is not instanced)
   UTATThiefLootEntryData* lootEntry = (lootInstanceData == nullptr) ? _TakeLootEntryObjectFromPool(lootIdentifier) : nullptr;
   
   // Create new instance if not found
   if (!lootEntry)
   {
      lootEntry = NewObject<UTATThiefLootEntryData>(owner);
      if (lootInstanceData != nullptr)
      {
         lootEntry->LootInstance = *lootInstanceData;
      }
      else
      {
         lootEntry->LootInstance = FTATLootInstance();
         lootEntry->LootInstance.Identifier = lootIdentifier;
      }
   }
   return lootEntry;
}

void FTATThiefLootEntryMgr::_ReturnLootEntryObjectToPool(UTATThiefLootEntryData* lootEntry)
{
   check(IsValid(lootEntry));
   bool isAlreadyInSet = false;
   // Reset state
   lootEntry->Count = 1;

   _lootEntryObjectPool.Add(lootEntry, &isAlreadyInSet);

   ensureMsgf(!isAlreadyInSet, TEXT("_ReturnLootEntryObjectToPool() | Loot entry under tag %s was already found in set!"), *lootEntry->LootInstance.Identifier.LootTag.ToString());
}

UTATThiefLootEntryData* FTATThiefLootEntryMgr::_TakeLootEntryObjectFromPool(const FTATLootIdentifier& lootIdentifier)
{
   // Search for an existing entry in the object pool
   for (auto it = _lootEntryObjectPool.CreateIterator(); it; ++it)
   {
      UTATThiefLootEntryData* objectPoolLootEntry = *it;
      check(IsValid(objectPoolLootEntry));
      if (objectPoolLootEntry->LootInstance.Identifier == lootIdentifier)
      {
         // Remove if found
         it.RemoveCurrent();
         return objectPoolLootEntry;
      }
   }

   return nullptr;
}

TSet<UTATThiefLootEntryData*>& FTATThiefLootEntryMgr::_GetLootEntrySetForType(ETATLootType lootType)
{
   switch (lootType)
   {
   case ETATLootType::MajorLoot:
      return _majorLootListEntries;
      break;
   case ETATLootType::MinorLoot:
      return _minorLootListEntries;
      break;
 
   // Should never get hit, but need to return something in all paths
   default:
      checkNoEntry();
      return _minorLootListEntries;
   }
}

void FTATThiefLootEntryMgr::RemoveListEntriesOfType(ETATLootType lootType)
{
   UE_LOG(LogTATThiefLootEntryMgr, Verbose, TEXT("Clearing %s loot entries..."), *UEnum::GetDisplayValueAsText(lootType).ToString());

   // Return entries to object pool
   TSet<UTATThiefLootEntryData*>& lootListEntries = _GetLootEntrySetForType(lootType);
   for (UTATThiefLootEntryData* lootEntry : lootListEntries)
   {
      _ReturnLootEntryObjectToPool(lootEntry);
   }

   // Clear set
   lootListEntries.Reset();
}
