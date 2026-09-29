// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "GameFramework/TATMatchPersistenceGameInstanceSubsystem.h"
#include "UI/TATScreenWidget.h"
#include "UI/ThiefTab/TATThiefLootEntryMgr.h"

#include "TATPostMatchResultsScreen.generated.h"

class UListView;
class ATATPlayerController;

USTRUCT(BlueprintType)
struct TAT_API FTATPostMatchScoreboardEntryData
{
   GENERATED_BODY()
public:
   UPROPERTY(BlueprintReadOnly)
   FMatchPersistentRankingPlayerData RankingData;

   UPROPERTY(BlueprintReadOnly)
   bool IsLootLeader = false;
};

UCLASS(Blueprintable, meta = (DisableNativeTick))
class TAT_API UTATPostMatchResultsScreen : public UTATScreenWidget
{
   GENERATED_BODY()

   // from UTATScreenWidget
   virtual void HandleOnScreenOnTopOfStack(ATATPlayerController& ownerPC) override;
   virtual void HandleOnScreenRemovedFromStack(ATATPlayerController& ownerPC) override;

public:
   UFUNCTION(BlueprintPure, BlueprintImplementableEvent, Category = "Loot")
   UListView* GetListViewForLootType(ETATLootType lootType) const;

   /// Update the UI to show if we escaped or not, and what our placement on the escape order was
   /// Escape order placement will be one-indexed, e.g. 1 for first, 2 for second, etc.
   UFUNCTION(BlueprintImplementableEvent, Category = "Loot")
   void SetEscapeOrderPlacement(bool didEscape, int escapeOrderPlacement);

   // Called with player scoreboard entries listed in order of most -> least loot
   UFUNCTION(BlueprintImplementableEvent, Category = "Ranking")
   void SetPlayerLootRankingData(const TArray<FTATPostMatchScoreboardEntryData>& scoreboardData);

   UFUNCTION(BlueprintPure, Category = "Player Stats", meta=(GameplayTagFilter="PlayerStats"))
   int GetPlayerStat(FGameplayTag playerStatTag) const;

   UFUNCTION(BlueprintPure, Category = "Coop")
   int GetAllyCarriedLootValue() const { return _matchPersistentData.AllyCarriedLootValue; }

   UFUNCTION(BlueprintPure, Category = "Coop")
   int GetStashedLootValue() const { return _matchPersistentData.StashedLootValue; }

   UFUNCTION(BlueprintPure, Category = "Coop")
   int GetCarriedLootValue() const { return _matchPersistentData.KeptCarriedLootValue; }

   UFUNCTION(BlueprintPure, Category = "Coop")
   int GetTotalLootValue() const { return _matchPersistentData.GetTotalValue(); }

   UFUNCTION(BlueprintPure, Category = "Coop")
   bool HasCoopAllies() const { return _matchPersistentData.HasCoopAllies; }

private:
   void InitFromMatchPersistentData(const FMatchPersistentData& matchPersistentData, const TArray<FMatchPersistentRankingPlayerData>& playerRankingData);
   
private:
   UPROPERTY(Transient)
   FTATThiefLootEntryMgr _lootEntryMgr;

   UPROPERTY(Transient)
   FMatchPersistentData _matchPersistentData;
};
