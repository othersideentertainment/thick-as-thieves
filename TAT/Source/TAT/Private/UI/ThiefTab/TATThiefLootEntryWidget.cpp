// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/ThiefTab/TATThiefLootEntryWidget.h"

// tat
#include "Developer/TATLootSettings.h"

#include "Loot/TATLootUtils.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATThiefLootEntryWidget)

void UTATThiefLootEntryWidget::NativeOnListItemObjectSet(UObject* listItemObject)
{
   const UTATThiefLootEntryData* thiefLootEntryData = CastChecked<UTATThiefLootEntryData>(listItemObject);
   SetLoot(thiefLootEntryData->LootInstance);
   FTATLootMetadataBP metadata;
   UTATLootUtils::FindLootMetadata(GetWorld(), GetLootInfo().LootIdentifier, metadata);
   // Notify BP to update visuals. 
   RefreshVisuals(metadata, thiefLootEntryData->Count, _lootInstance);
}

const FTATLootInfo& UTATThiefLootEntryWidget::GetLootInfo() const
{
   return UTATLootSettings::Get().GetLootInfoChecked(this, _lootInstance.Identifier);
}

void UTATThiefLootEntryWidget::SetLoot(const FTATLootIdentifier& lootIdentifier)
{
   check(lootIdentifier.IsValid());
   _lootInstance = FTATLootInstance();
   _lootInstance.Identifier = lootIdentifier;
}

void UTATThiefLootEntryWidget::SetLoot(const FTATLootInstance& lootInstance)
{
   check(lootInstance.Identifier.IsValid());
   _lootInstance = lootInstance;
}

const FTATLootInfo& UTATThiefLootEntryData::GetLootInfo() const
{
   return UTATLootSettings::Get().GetLootInfoChecked(this, LootInstance.Identifier);
}
