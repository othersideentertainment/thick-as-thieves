// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Developer/TATToolSettings.h"

// tat
#include "Tools/TATToolComponent.h"
#include "Tools/TATToolTypes.h"

// ose
#include "Abilities/OSEUpgradeState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATToolSettings)

// static
TSoftClassPtr<UTATToolComponent> UTATToolSettings::LookupToolClassByToolType(const UDataTable* gearMetadata, FGameplayTag gearTag, const FUpgradeState* characterUpgradeState)
{
   if (const FTATGearMetadataTableRow* row = UTATToolSettings::Get().FindGearMetadata(gearTag, gearMetadata))
   {
      check(row->ToolID == gearTag);
      if (row->ToolUpgrade.IsValid() && characterUpgradeState != nullptr && characterUpgradeState->GetValue(row->ToolUpgrade.UpgradeTag) >= row->ToolUpgrade.UpgradeLevel)
      {
         return row->ToolUpgrade.UpgradeToolClass;
      }
      return row->ToolClass;
   }
   return TSoftClassPtr<UTATToolComponent>{};
}

const FTATGearMetadataTableRow* UTATToolSettings::FindGearMetadata(FGameplayTag toolId, const UDataTable* gearDataTable) const
{
   if (gearDataTable == nullptr)
   {
      gearDataTable = _GetGearDataTable();
   }

   // Rebuild the id to row map if needed
   if (gearDataTable != nullptr && gearDataTable->GetRowMap().Num() != _toolDataTableMap.Num())
   {
      _toolDataTableMap.Reset(gearDataTable, [](const FTATGearMetadataTableRow& gearInfo) { return gearInfo.ToolID; });
   }

   return _toolDataTableMap.Find(gearDataTable, toolId);
}

void UTATToolSettings::ForEachGearMetadataRow(TFunctionRef<void(const FTATGearMetadataTableRow&)> callback, const UDataTable* gearDataTable) const
{
   if (gearDataTable == nullptr)
   {
      gearDataTable = _GetGearDataTable();
   }

   if (gearDataTable != nullptr)
   {
      static const TCHAR* contextString = TEXT("UTATToolSettings::ForEachGearMetadataRow");
      gearDataTable->ForeachRow<FTATGearMetadataTableRow>(contextString, [callback](const FName&, const FTATGearMetadataTableRow& value)
      {
         callback(value);
      });
   }
}

// static
bool UTATToolSettings::BP_FindGearMetadata(FGameplayTag toolId, FTATGearMetadataTableRow& gearMetadata)
{
   if (const FTATGearMetadataTableRow* row = UTATToolSettings::Get().FindGearMetadata(toolId))
   {
      gearMetadata = *row;
      return true;
   }
   gearMetadata = FTATGearMetadataTableRow{};
   return false;
}

const UDataTable* UTATToolSettings::_GetGearDataTable() const
{
   UDataTable* gearDataTable = GearMetadataTable.LoadSynchronous();
   check(gearDataTable != nullptr);
   return gearDataTable;
}
