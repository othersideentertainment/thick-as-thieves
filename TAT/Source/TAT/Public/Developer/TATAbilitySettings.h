// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "CharacterCustomization/TATCharacterLoadout.h"
#include "Developer/TATDataTableMap.h"

// ose
#include "Abilities/OSEAbilityInputBinds.h"

// ue
#include "GameplayTagContainer.h"
#include "Engine/DeveloperSettings.h"

#include "TATAbilitySettings.generated.h"

class UDataTable;
class UGameplayAbility;
struct FTATAbilityLoadoutMetadataTableRow;
struct FUpgradeState;

USTRUCT()
struct FTATAbilityLoadoutInputConfig
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere)
   EAbilityInputType AbilityInput = EAbilityInputType::None;
};

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[TAT] Ability Settings"))
class TAT_API UTATAbilitySettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   static const UTATAbilitySettings& Get() { return *GetDefault<UTATAbilitySettings>(); }

   const FTATAbilityLoadoutMetadataTableRow* FindAbilityMetadata(FGameplayTag abilityId, const UDataTable* abilityDataTable = nullptr) const;

   void ForEachAbilityMetadataRow(TFunctionRef<void(const FTATAbilityLoadoutMetadataTableRow&)> callback, const UDataTable* abilityDataTable = nullptr) const;

   /// Retrieves the metadata for an ability that can be selected by players for use in loadouts
   UFUNCTION(BlueprintCallable, DisplayName = "Find Ability Metadata (For Loadouts)", Category = "Ability Settings")
   static bool BP_FindAbilityMetadata(FGameplayTag abilityId, FTATAbilityLoadoutMetadataTableRow& abilityMetadata);

   /// The data table containing abilities that are player-selectable in loadouts
   UPROPERTY(Config, EditAnywhere, Category = "Ability Loadout", Meta=(RowType="/Script/TAT.TATAbilityMetadataTableRow"))
   TSoftObjectPtr<UDataTable> AbilityLoadoutMetadataTable;

   /// Maps player-facing input action types to actual inputs
   UPROPERTY(Config, EditAnywhere, Category = "Ability Loadout")
   TMap<ETATCharacterInputActionType, FTATAbilityLoadoutInputConfig> AbilityLoadoutInputMapping;

private:
   const UDataTable* _GetAbilityDataTable() const;

   // Lookup map to make finding ability data table entries by gameplay tag fast (mutable so it can lazily populate the mapping in const lookup functions)
   mutable TTATDataTableMap<FTATAbilityLoadoutMetadataTableRow> _abilityDataTableMap;
};
