// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"

#include "TATAbilityLoadoutTypes.generated.h"

class UPaperSprite;
class UGameplayAbility;
struct FUpgradeState;
struct FTATAbilityLoadoutInputConfig;

/// Represents metadata for an ability that can be placed in a player's loadout
USTRUCT(BlueprintType)
struct TAT_API FTATAbilityLoadoutMetadata
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FText AbilityName;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FText AbilityDescription;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayThumbnail = "true"))
   TSoftObjectPtr<UPaperSprite> IconSprite;
};

USTRUCT(BlueprintType)
struct FTATAbilityLoadoutUpgrade
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Upgrade")
   FGameplayTag UpgradeTag;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Upgrade", Meta = (UIMin = 1, ClampMin = 1))
   int32 UpgradeLevel = 1;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Upgrade")
   TSoftClassPtr<UGameplayAbility> UpgradeAbilityClass;

   bool IsValid() const { return UpgradeTag.IsValid() && !UpgradeAbilityClass.IsNull(); }
};

USTRUCT(BlueprintType)
struct TAT_API FTATAbilityLoadoutMetadataTableRow : public FTableRowBase
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (Categories = "Ability.Type"))
   FGameplayTag AbilityId;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FTATAbilityLoadoutMetadata Metadata;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TSoftClassPtr<UGameplayAbility> AbilityClass;

   /// Category the ability is in. Defines which loadout slots the ability can be placed in.
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (Categories = "Loadout.Slot"))
   FGameplayTag LoadoutSlotCategory;

   /// A character with this upgrade tag and level will get this ability instead of the normal ability class.
   /// Useful for less granular upgrades (eg. an upgrade that completely replaces another ability)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FTATAbilityLoadoutUpgrade AbilityUpgrade;

   /// Gets the ability class for this ability, optionally taking the character's upgrade state into account
   TSoftClassPtr<UGameplayAbility> GetAbilityClass(const FUpgradeState* characterUpgradeState = nullptr) const;

#if WITH_EDITOR
   // NB: The data table itself also has a OnDataTableChanged delegate, however when using FDataTableRowHandle, we only have
   // a const pointer to the data table, but can get a mutable pointer to the individual row
   DECLARE_MULTICAST_DELEGATE(FOnMetadataDataTableChanged);
   FOnMetadataDataTableChanged OnMetadataDataTableChanged;

   virtual void OnDataTableChanged(const UDataTable* dataTable, const FName rowName) override;
#endif
};
