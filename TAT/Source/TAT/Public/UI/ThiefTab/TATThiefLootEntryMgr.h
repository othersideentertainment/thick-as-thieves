// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Loot/TATLootTypes.h"

// ue
#include "Containers/Set.h"

#include "TATThiefLootEntryMgr.generated.h"

enum class ETATLootType : uint8;
class UListView;
class UTATThiefLootEntryData;

// Container used for storage/lookup of loot entry widgets
USTRUCT()
struct TAT_API FTATThiefLootEntryMgr
{
   GENERATED_BODY()

public:
   // Constructs a proxy object for UListView usage, adding to list view and caching in collection
   inline void ConstructListEntry(const FTATLootIdentifier& lootIdentifier, UListView* listView) { _ConstructListEntry(lootIdentifier, listView, nullptr); }
   inline void ConstructListEntry(const FTATLootInstance& lootInstance, UListView* listView) { _ConstructListEntry(lootInstance.Identifier, listView, &lootInstance); }

   void RemoveLootListEntry(UTATThiefLootEntryData* lootEntry);
   void RemoveListEntriesOfType(ETATLootType lootType);

   void ClearLootListEntries();

private:
   void _ConstructListEntry(const FTATLootIdentifier& lootIdentifier, UListView* listView, const FTATLootInstance* lootInstanceData);

   // Searches the object pool for a UTATThiefLootEntryData with matching LootIdentifier, returning if found (otherwise creating a new one under the given owner)
   UTATThiefLootEntryData* _FindOrCreateLootEntryObject(const FTATLootIdentifier& lootIdentifier, UObject* owner, const FTATLootInstance* lootInstanceData);

   // Returns the given UTATThiefLootEntryData to the object pool, ensuring it isn't already present
   void _ReturnLootEntryObjectToPool(UTATThiefLootEntryData* lootEntry);

   // Searches the object pool for a loot entry of matching identifier, removing it from the pool if found
   UTATThiefLootEntryData* _TakeLootEntryObjectFromPool(const FTATLootIdentifier& lootIdentifier);

   TSet<UTATThiefLootEntryData*>& _GetLootEntrySetForType(ETATLootType lootType);

private:
   UPROPERTY(Transient)
   TSet<UTATThiefLootEntryData*> _minorLootListEntries;

   UPROPERTY(Transient)
   TSet<UTATThiefLootEntryData*> _majorLootListEntries;

   // Object pool of list entry objects, available for reuse across the owning widget's lifetime
   UPROPERTY(Transient)
   TSet<UTATThiefLootEntryData*> _lootEntryObjectPool;
};
