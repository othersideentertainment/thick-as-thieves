// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "TATUpgradeType.generated.h"

class UPaperSprite;

/// How an upgrade is treated with regards to player progression
UENUM(BlueprintType)
enum class ETATUpgradeProgressionType : uint8
{
   None,
   MinorUpgrade,
   MajorUpgrade,
   TierUpgrade,
   MAX UMETA(Hidden)
};

UENUM(BlueprintType)
enum class ETATUpgradePurchaseState : uint8
{
   Unknown,
   Locked         UMETA(Tooltip = "Some or all prereqs are missing"),
   Available      UMETA(Tooltip = "Prereq upgrades are present but not all currency requirements are met"),
   Purchaseable   UMETA(Tooltip = "Available for purchase (all prereqs are met)"),
   Unlocked       UMETA(Tooltip = "The upgrade has been purchased"),
};

/// The base data for an upgrade type that can be used in an upgrade graph.
/// This primarily consists of a gameplay tag that represents the upgrade in save data as well as any player-facing metadata (eg. localized display name).
/// This is sort of like the "class" that represents an upgrade type, and upgrade graph nodes are "instances" of the class.
UCLASS(BlueprintType)
class TAT_API UTATUpgradeType : public UDataAsset
{
   GENERATED_BODY()

public:
   UTATUpgradeType();

   /// Player-facing name of the upgrade. Can be overridden when placed in upgrade graphs.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
   FText Name;

   /// Player-facing description of the upgrade. Can be overridden when placed in upgrade graphs.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display", Meta = (MultiLine = true))
   FText Description;

   /// Player-facing icon for the upgrade. Can be overridden when placed in upgrade graphs.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
   TSoftObjectPtr<UPaperSprite> Icon;

   /// What category this upgrade is in if it's used for player progression.
   /// Some types have limits on how many can be unlocked at any given time.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
   ETATUpgradeProgressionType ProgressionType = ETATUpgradeProgressionType::None;

   /// The tag to apply to the player when the upgrade is unlocked.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Output", meta = (Categories = "Upgrade"))
   FGameplayTag UpgradeTag;

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR
};
