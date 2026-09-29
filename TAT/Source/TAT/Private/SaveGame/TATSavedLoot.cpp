// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "SaveGame/TATSavedLoot.h"

// tat
#include "Developer/TATLootSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSavedLoot)
DEFINE_LOG_CATEGORY_STATIC(LogTATSavedLoot, Log, All);

namespace SavedLootHelpers
{
   void AddToStacks(TArray<FTATSavedLootStack>& stacks, FTATLootIdentifier lootId, int32 count = 1)
   {
      if (FTATSavedLootStack* stack = stacks.FindByKey(lootId))
      {
         stack->Count += count;
      }
      else
      {
         stacks.Emplace(lootId, count);
      }
   }

   void AddToStacks(TMap<FTATLootIdentifier, int32>& stacks, FTATLootIdentifier lootId, int32 count = 1)
   {
      stacks.FindOrAdd(lootId) += count;
   }
}

void FTATSavedLootStack::AppendToString(FString& string) const
{
   string.Appendf(TEXT("(%s Count: %d)"), *LootIdentifier.LootTag.ToString(), Count);
}

void FTATSavedLootAddRequest::PopulateFrom(TConstArrayView<FTATLootIdentifier> lootArray)
{
   for (FTATLootIdentifier lootId : lootArray)
   {
      SavedLootHelpers::AddToStacks(Stacks, lootId);
   }
}

void FTATSavedLootAddRequest::AddCount(FTATLootIdentifier lootId, int32 count, UObject* contextObject)
{
   // CONSIDER: remove this lookup?
   const FTATLootInfo* lootInfo = UTATLootSettings::Get().GetLootInfo(contextObject, lootId);
   if (ensure(lootInfo))
   {
      SavedLootHelpers::AddToStacks(Stacks, lootId, count);
   }
}

void FTATSavedLootInventory::Init()
{
}

void FTATSavedLootInventory::Add(const FTATSavedLootAddRequest& request)
{
   for (const FTATLootCountPair& stack : request.Stacks)
   {
      SavedLootHelpers::AddToStacks(Stacks, stack.Key, stack.Value);
   }
}

bool FTATSavedLootInventory::Remove(const FTATSavedLootRemoveRequest& request)
{
   // Check to make sure that everything requested to be removed we actually have
   // If not, don't remove anything and return false as a notification to callee
   if (!ValidateLootForRemove(request))
   {
      return false;
   }

   for (const FTATLootCountPair& stack : request.Stacks)
   {
      const FTATLootIdentifier& stackID = stack.Key;
      int32* count = Stacks.Find(stackID);
      check(count);

      const int32 oldCount = *count;
      const int32 newCount = FMath::Max(0, oldCount - stack.Value);

      if (newCount == 0)
      {
         Stacks.Remove(stackID);
      }
      else if (oldCount != newCount)
      {
         *count = newCount;
      }
   }

   return true;
}

bool FTATSavedLootInventory::ValidateLootForRemove(const FTATSavedLootRemoveRequest& lootToRemove) const
{
   for (auto itStackToRemove = lootToRemove.Stacks.CreateConstIterator(); itStackToRemove; ++itStackToRemove)
   {
      const FTATLootCountPair& stackToRemove = *itStackToRemove;
      
      // if not found, will just return zero which should be OK for the following logic
      const int32 maxValidCount = Stacks.FindRef(stackToRemove.Key); 
      int32 invalidCount = stackToRemove.Value - maxValidCount;
      if (invalidCount > 0)
      {
         return false;
      }
   }

   return true;
}

void FTATSavedLootInventory::AppendToString(FString& string) const
{
   string.Append(TEXT("\nLootStacks:"));
   for (const FTATLootCountPair& stack : Stacks)
   {
      string.Append(TEXT("\n"));
      FTATSavedLootStack(stack.Key, stack.Value).AppendToString(string);
   }
}

void UTATSavedLootUtils::GetLootStacksAsArray(const FTATSavedLootInventory& inventory, TArray<FTATSavedLootStack>& outStacks)
{
   outStacks.Reset(inventory.GetStacks().Num());
   for (const FTATLootCountPair& stack : inventory.GetStacks())
   {
      outStacks.Emplace(stack.Key, stack.Value);
   }
}
