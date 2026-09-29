// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Upgrades/TATUpgradeType.h"

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TATCharacterUpgradeUtils.generated.h"

class UTATUpgradeGraph;
class UTATUpgradeGraphNode;
struct FOSEGenericGraphNodeHandle;
struct FTATCharacterDataContext;

UCLASS()
class TAT_API UTATCharacterUpgradeUtils : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   /// Gets the upgrade level for an upgrade tag
   UFUNCTION(BlueprintPure, Category = "Character Upgrade Utils")
   static int32 GetCharacterUpgradeLevelForTag(const FTATCharacterDataContext& context, FGameplayTag upgradeTag);

   /// Gets the upgrade level for an upgrade node
   UFUNCTION(BlueprintPure, Category = "Character Upgrade Utils")
   static int32 GetCharacterUpgradeLevelForNode(const FTATCharacterDataContext& context, const FOSEGenericGraphNodeHandle& upgradeNodeHandle);

   UFUNCTION(BlueprintPure, Category = "Character Upgrade Utils")
   static bool IsCharacterUpgradeNodeUnlocked(const FTATCharacterDataContext& context, const FOSEGenericGraphNodeHandle& upgradeNodeHandle);

   UFUNCTION(BlueprintPure, Category = "Character Upgrade Utils")
   static int32 GetNumUnlockedUpgradesWithProgressionType(const FTATCharacterDataContext& context, UTATUpgradeGraph* upgradeGraph, ETATUpgradeProgressionType progressionType);

   /// Checks if an upgrade is unlocked, locked, purchasable, etc.
   /// If you just need to know if it's unlocked, use IsUpgradeUnlocked instead, as it's much faster (this function checks all prereqs)
   UFUNCTION(BlueprintCallable, Category = "Character Upgrade Utils")
   static ETATUpgradePurchaseState GetCharacterUpgradePurchaseState(const FTATCharacterDataContext& context, const FOSEGenericGraphNodeHandle& upgradeHandle);

   /// Attempts to perform an upgrade, returning true if all prereqs were met and the upgrade was added successfully.
   /// If not all prereqs were met, returns false (making no changes).
   UFUNCTION(BlueprintCallable, Category = "Character Upgrade Utils")
   static bool RequestUnlockCharacterUpgrade(const FTATCharacterDataContext& context, const FOSEGenericGraphNodeHandle& upgradeNodeHandle, bool autoEquip = true);

   /// Refunds all money and upgrade currency costs associated with an upgrade node.
   /// Returns true if anything was refunded.
   static bool RefundUpgradeNode(const FTATCharacterDataContext& context, UTATUpgradeGraphNode* upgradeNode);

   /// only for testing, probably
   UFUNCTION(BlueprintCallable, Category = "Character Upgrade Utils")
   static void ResetCharacterUpgradeProgressForNode(const FTATCharacterDataContext& context, const FOSEGenericGraphNodeHandle& upgradeNodeHandle);

   /// Removes all upgrades for the specified character, optionally refunding all currency needed to repurchase those upgrades.
   UFUNCTION(BlueprintCallable, Category = "Character Upgrade Utils")
   static void ResetAllCharacterUpgradeProgress(const FTATCharacterDataContext& context, UTATUpgradeGraph* upgradeGraph, bool refundUpgradeCosts);

   /// Gets the upgrade graph assigned to the specified character save slot
   UFUNCTION(BlueprintPure, Category = "Character Upgrade Utils")
   static TSoftObjectPtr<UTATUpgradeGraph> GetCharacterUpgradeGraph(const FTATCharacterDataContext& context);
};
