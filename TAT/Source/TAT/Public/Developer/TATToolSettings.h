// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Developer/TATDataTableMap.h"

// ue5
#include "GameplayTagContainer.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/StaticMesh.h"

#include "TATToolSettings.generated.h"

class UDataTable;
class UTATToolComponent;
struct FTATGearMetadataTableRow;
struct FUpgradeState;

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[TAT] Tool Settings"))
class TAT_API UTATToolSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   static const UTATToolSettings& Get() { return *GetDefault<UTATToolSettings>(); }

   static TSoftClassPtr<UTATToolComponent> LookupToolClassByToolType(const UDataTable* gearMetadata, FGameplayTag gearTag, const FUpgradeState* characterUpgradeState = nullptr);

   const FTATGearMetadataTableRow* FindGearMetadata(FGameplayTag toolId, const UDataTable* gearDataTable = nullptr) const;

   void ForEachGearMetadataRow(TFunctionRef<void(const FTATGearMetadataTableRow&)> callback, const UDataTable* gearDataTable = nullptr) const;

   UFUNCTION(BlueprintCallable, DisplayName = "Find Gear Metadata", Category = "Tool Settings")
   static bool BP_FindGearMetadata(FGameplayTag toolId, FTATGearMetadataTableRow& gearMetadata);

   /// Interact prompt for when the player is picking up a deployed world actor
   UPROPERTY(Config, EditAnywhere, Category = "Interaction|Prompts")
   FText PickupDeployablePrompt;

   /// Interaction prompt for when the player is destroying a deployed world actor
   UPROPERTY(Config, EditAnywhere, Category = "Interaction|Prompts")
   FText DestroyDeployablePrompt;

   /// Interact error prompt for when the player is trying to pick up a deployed world actor,
   /// but can't due to having too much ammo already
   UPROPERTY(Config, EditAnywhere, Category = "Interaction|Prompts")
   FText CannotPickupDeployableDueToAmmoLimitPrompt;

   /// How long the interact hold for picking up deployables lasts
   UPROPERTY(Config, EditAnywhere, Category = "Interaction")
   float PickupDeployableHoldDuration = 2.0f;

   /// Animation to play while picking up a deployable
   UPROPERTY(Config, EditAnywhere, Category = "Interaction")
   FGameplayTag PickupDeployableHoldAnimationTag;

   UPROPERTY(Config, EditAnywhere, Category = "Gear", Meta=(RowType="/Script/TAT.TATGearMetadataTableRow"))
   TSoftObjectPtr<UDataTable> GearMetadataTable;

   UPROPERTY(Config, EditAnywhere, Category = "Editor")
   FGameplayTagContainer TagsToBlockActivationForToolUsageAbilities;

   UPROPERTY(Config, EditAnywhere, Category = "Editor")
   FGameplayTagContainer RequiredAbilityTagsForToolUsageAbilities;

   // Additional time for server to wait for the next swing before authoritatively ending a chain attack in progress. NOTE: not applied for listen server host
   UPROPERTY(Config, EditAnywhere, Category = "Editor")
   float ChainAttackServerFudgeWindowSeconds = 0.5f;

private:
   const UDataTable* _GetGearDataTable() const;

   // Lookup map to make finding gear data table entries by gameplay tag fast (mutable so it can lazily populate the mapping in const lookup functions)
   mutable TTATDataTableMap<FTATGearMetadataTableRow> _toolDataTableMap;
};
