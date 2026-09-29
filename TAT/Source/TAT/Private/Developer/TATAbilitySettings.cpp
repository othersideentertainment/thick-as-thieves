// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Developer/TATAbilitySettings.h"

// tat
#include "Abilities/TATAbilityLoadoutTypes.h"

// ose
#include "Abilities/OSEUpgradeState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAbilitySettings)

const FTATAbilityLoadoutMetadataTableRow* UTATAbilitySettings::FindAbilityMetadata(FGameplayTag abilityId, const UDataTable* abilityDataTable) const
{
   if (abilityDataTable == nullptr)
   {
      abilityDataTable = _GetAbilityDataTable();
   }

   // Rebuild the id to row map if needed
   if (abilityDataTable != nullptr && abilityDataTable->GetRowMap().Num() != _abilityDataTableMap.Num())
   {
      _abilityDataTableMap.Reset(abilityDataTable, [](const FTATAbilityLoadoutMetadataTableRow& abilityInfo) { return abilityInfo.AbilityId; });
   }

   return _abilityDataTableMap.Find(abilityDataTable, abilityId);
}

void UTATAbilitySettings::ForEachAbilityMetadataRow(TFunctionRef<void(const FTATAbilityLoadoutMetadataTableRow&)> callback, const UDataTable* abilityDataTable) const
{
   if (abilityDataTable == nullptr)
   {
      abilityDataTable = _GetAbilityDataTable();
   }

   if (abilityDataTable != nullptr)
   {
      static const TCHAR* contextString = TEXT("UTATAbilitySettings::ForEachAbilityMetadataRow");
      abilityDataTable->ForeachRow<FTATAbilityLoadoutMetadataTableRow>(contextString, [callback](const FName&, const FTATAbilityLoadoutMetadataTableRow& value)
      {
         callback(value);
      });
   }
}

// static
bool UTATAbilitySettings::BP_FindAbilityMetadata(FGameplayTag abilityId, FTATAbilityLoadoutMetadataTableRow& abilityMetadata)
{
   if (const FTATAbilityLoadoutMetadataTableRow* row = UTATAbilitySettings::Get().FindAbilityMetadata(abilityId))
   {
      abilityMetadata = *row;
      return true;
   }
   abilityMetadata = FTATAbilityLoadoutMetadataTableRow{};
   return false;
}

const UDataTable* UTATAbilitySettings::_GetAbilityDataTable() const
{
   UDataTable* abilityDataTable = AbilityLoadoutMetadataTable.LoadSynchronous();
   check(abilityDataTable != nullptr);
   return abilityDataTable;
}
