// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Loot/TATLootTypes.h"

// tat
#include "Damage/TATDamageTypes.h"
#include "Developer/TATLootSettings.h"
#include "Loot/TATLootInstanceIDSubsystem.h"
#include "Loot/TATLootUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLootTypes)

void FTATLootIdentifier::Invalidate()
{
   LootTag = FGameplayTag::EmptyTag;
}

FString FTATLootIdentifier::ToString() const
{
   return LootTag.IsValid()
      ? FString::Printf(TEXT("LootIdentifier(%s)"), *LootTag.ToString())
      : TEXT("LootIdentifier(Invalid)");
}

bool FTATLootIdentifier::NetSerialize(FArchive& ar, class UPackageMap* map, bool& outSuccess)
{
   return LootTag.NetSerialize(ar, map, outSuccess);
}

void FTATLootInstance::Invalidate()
{
   Identifier.Invalidate();
   Id.Invalidate();
}

FString FTATLootInstance::ToString() const
{
   if (!Identifier.IsValid() || !Id.IsValid())
   {
      return TEXT("LootInstance(Invalid)");
   }

   return FString::Printf(
      TEXT("LootInstance(ID=%d, Type=%s)"),
      Id.Id,
      *Identifier.LootTag.ToString());
}

int32 FTATLootInfo::GetSlotSize() const
{
   return UseSlotSizeOverride ? SlotSizeOverride : UTATLootSettings::Get().DefaultLootSlotSize[static_cast<int32>(LootType)];
}

bool FTATLootInfo::RequiresInstanceStorage() const
{
   //NB. If you add any fields to FTATLootInstance, add additional checks here to determine if a loot item needs to use those fields
   return LootType == ETATLootType::MajorLoot;
}

FTATLootInstance FTATLootInfo::CreateDefaultInstance(const UObject* contextObj) const
{
   ensureMsgf(RequiresInstanceStorage(), TEXT("FTATLootInfo::CreateDefaultInstance called on non-instance loot '%s'"), *LootIdentifier.LootTag.ToString());

   FTATLootInstance result;
   result.Identifier = LootIdentifier;

   UTATLootInstanceIDSubsystem* idSubsystem = UTATLootInstanceIDSubsystem::Get(contextObj);
   if (ensure(idSubsystem))
   {
      result.Id = FTATLootInstanceId(idSubsystem->AuthorityGetNextLootID());
   }

   return result;
}

FTATLootMetadataBP::FTATLootMetadataBP(const FTATLootInfo& lootInfo)
   : LootIdentifier(lootInfo.LootIdentifier)
   , LootType(lootInfo.LootType)
   , SlotSize(lootInfo.GetSlotSize())
   , DisplayName(lootInfo.DisplayName)
   , DisplaySprite(lootInfo.DisplaySprite)
   , LootValue(lootInfo.Value)
{
}

FTATLootContainer FTATLootContainer::Copy() const
{
   FTATLootContainer lootContainer;

   // Copy over loot data
   lootContainer.Items = Items;
   lootContainer.Instances = Instances;

   // Explicitly avoid copying delegate bindings!

   return lootContainer;
}

void FTATLootContainer::Add(const FTATLootIdentifier& lootId)
{
   check(lootId.IsValid());
   Items.Add(lootId);
}

void FTATLootContainer::Add(const FTATLootInstance& lootInst)
{
   check(lootInst.IsValid());
   Instances.Add(lootInst);
}

void FTATLootContainer::Add(const FTATLootItemVariant& lootItem)
{
   if (const FTATLootInstance* lootInstance = lootItem.TryGetInstance())
   {
      Add(*lootInstance);
   }
   else
   {
      Add(lootItem.GetIdentifier());
   }
}

void FTATLootContainer::Append(const TArray<FTATLootIdentifier>& items)
{
   for(const FTATLootIdentifier lootIdentifier : items)
   {
      Add(lootIdentifier);
   }
}

void FTATLootContainer::Append(const TArray<FTATLootInstance>& instances)
{
   for(const FTATLootInstance& lootInstance : instances)
   {
      Add(lootInstance);
   }
}

int32 FTATLootContainer::RemoveItemSingle(const FTATLootIdentifier& lootId)
{
   check(lootId.IsValid());
   
   return Items.RemoveSingle(lootId);
}

void FTATLootContainer::RemoveItemAtSwap(int32 lootIndex)
{
   check(Items.IsValidIndex(lootIndex));
   Items.RemoveAtSwap(lootIndex);
}

void FTATLootContainer::RemoveInstanceAt(int32 lootIndex)
{
   check(Instances.IsValidIndex(lootIndex));

   const FTATLootInstance lootInstance = Instances[lootIndex];
   check(lootInstance.IsValid());

   Instances.RemoveAt(lootIndex);
}

void FTATLootContainer::RemoveInstanceAtSwap(int32 lootIndex)
{
   check(Instances.IsValidIndex(lootIndex));
   Instances.RemoveAtSwap(lootIndex);
}

void FTATLootContainer::ResetItems()
{
   for (int32 idx = 0; idx < NumItems(); ++idx)
   {
      RemoveItemAtSwap(0);
   }
}

void FTATLootContainer::ResetInstances()
{
   for (int32 idx = 0; idx < NumInstances(); ++idx)
   {
      RemoveInstanceAtSwap(0);
   }
}

int32 FTATLootContainer::GetAmountOfLootType(const UObject* contextObject, ETATLootType lootType) const
{
   // NOTE: currently we only store minor loot in Items and major loot in Instances, but we check both
   // to avoid baking the "no runtime state for minor loot" design assumption into the architecture
   int32 count = 0;
   for (const FTATLootIdentifier& lootIdentifier : Items)
   {
      if (UTATLootUtils::GetLootType(contextObject, lootIdentifier) == lootType)
      {
         ++count;
      }
   }

   for (const FTATLootInstance& lootInstance : Instances)
   {
      if (UTATLootUtils::GetLootType(contextObject, lootInstance.Identifier) == lootType)
      {
         ++count;
      }
   }

   return count;
}

FTATLootIdentifier FTATLootContainer::Get(int32 containerIndex, FTATLootItemVariant* outItemVariant) const
{
   bool isInstance = false;
   const int32 index = _ContainerIndexToArrayIndex(containerIndex, isInstance);
   if (index == INDEX_NONE)
   {
      if (outItemVariant != nullptr)
      {
         outItemVariant->Invalidate();
      }
      return FTATLootIdentifier{};
   }

   if (isInstance)
   {
      if (outItemVariant != nullptr)
      {
         outItemVariant->Set(Instances[index]);
      }
      return Instances[index].Identifier;
   }
   else
   {
      if (outItemVariant != nullptr)
      {
         outItemVariant->Set(Items[index]);
      }
      return Items[index];
   }
}

FTATLootIdentifier FTATLootContainer::RemoveAt(int32 containerIndex, FTATLootItemVariant* outItemVariant)
{
   bool isInstance = false;
   const int32 index = _ContainerIndexToArrayIndex(containerIndex, isInstance);
   if (index == INDEX_NONE)
   {
      if (outItemVariant != nullptr)
      {
         outItemVariant->Invalidate();
      }
      return FTATLootIdentifier{};
   }

   if (isInstance)
   {
      if (outItemVariant != nullptr)
      {
         outItemVariant->Set(Instances[index]);
      }
      const FTATLootIdentifier result = Instances[index].Identifier;
      Instances.RemoveAt(index);
      return result;
   }
   else
   {
      if (outItemVariant != nullptr)
      {
         outItemVariant->Set(Items[index]);
      }
      const FTATLootIdentifier result = Items[index];
      Items.RemoveAt(index);
      return result;
   }
}

const FTATLootIdentifier& FTATLootContainer::operator[](int32 containerIndex) const
{
   bool isInstance = false;
   const int32 idx = _ContainerIndexToArrayIndex(containerIndex, isInstance);
   checkf(idx != INDEX_NONE, TEXT("Invalid container index %i into FTATLootContainer of size %i"), containerIndex, Num());
   return isInstance ? Instances[idx].Identifier : Items[idx];
}

int32 FTATLootContainer::_ContainerIndexToArrayIndex(int32 containerIndex, bool& outIsInstance) const
{
   if (containerIndex < 0)
   {
      return INDEX_NONE;
   }

   if (containerIndex < Instances.Num())
   {
      outIsInstance = true;
      return containerIndex;
   }

   containerIndex -= Instances.Num();
   if (containerIndex < Items.Num())
   {
      outIsInstance = false;
      return containerIndex;
   }

   return INDEX_NONE;
}
