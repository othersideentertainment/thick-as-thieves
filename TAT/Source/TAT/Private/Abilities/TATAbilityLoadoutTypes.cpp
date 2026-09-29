// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/TATAbilityLoadoutTypes.h"

// ose
#include "Abilities/OSEUpgradeState.h"
#include "Developer/TATAbilitySettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAbilityLoadoutTypes)

TSoftClassPtr<UGameplayAbility> FTATAbilityLoadoutMetadataTableRow::GetAbilityClass(const FUpgradeState* characterUpgradeState) const
{
   if (AbilityUpgrade.IsValid() && characterUpgradeState != nullptr && characterUpgradeState->GetValue(AbilityUpgrade.UpgradeTag) >= AbilityUpgrade.UpgradeLevel)
   {
      return AbilityUpgrade.UpgradeAbilityClass;
   }
   return AbilityClass;
}

#if WITH_EDITOR
void FTATAbilityLoadoutMetadataTableRow::OnDataTableChanged(const UDataTable* dataTable, const FName rowName)
{
   OnMetadataDataTableChanged.Broadcast();
}
#endif
