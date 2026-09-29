// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/PostMatch/TATPostMatchResultsScreen.h"

// tat
#include "Developer/TATLootSettings.h"
#include "Player/TATPlayerController.h"

// ue
#include "Components/ListView.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPostMatchResultsScreen)

void UTATPostMatchResultsScreen::HandleOnScreenOnTopOfStack(ATATPlayerController& ownerPC)
{
   if (UTATMatchPersistenceGameInstanceSubsystem* matchPersistenceSubsystem = ownerPC.GetGameInstance()->GetSubsystem<UTATMatchPersistenceGameInstanceSubsystem>())
   {
      const FMatchPersistentData& matchPersistentData = matchPersistenceSubsystem->GetMatchPersistentData();
      const TArray<FMatchPersistentRankingPlayerData>& playerRankingData = matchPersistenceSubsystem->GetPersistentPlayerRankingData();
      InitFromMatchPersistentData(matchPersistentData, playerRankingData);
   }
   
   Super::HandleOnScreenOnTopOfStack(ownerPC);
}

void UTATPostMatchResultsScreen::HandleOnScreenRemovedFromStack(ATATPlayerController& ownerPC)
{
   _lootEntryMgr.ClearLootListEntries();

   Super::HandleOnScreenRemovedFromStack(ownerPC);
}

int UTATPostMatchResultsScreen::GetPlayerStat(FGameplayTag playerStatTag) const
{
   return _matchPersistentData.PlayerStats.FindStatValue(playerStatTag);
}

void UTATPostMatchResultsScreen::InitFromMatchPersistentData(const FMatchPersistentData& matchPersistentData, const TArray<FMatchPersistentRankingPlayerData>& playerRankingData)
{
   // We should have set this to captured or escaped by this point
   ensure(matchPersistentData.CompletionState != EMatchCompletionState::Unset);

   _matchPersistentData = matchPersistentData;

   UListView* majorLootListView = GetListViewForLootType(ETATLootType::MajorLoot);
   UListView* minorLootListView = GetListViewForLootType(ETATLootType::MinorLoot);
   check(IsValid(majorLootListView));
   check(IsValid(minorLootListView));

   majorLootListView->ClearListItems();
   minorLootListView->ClearListItems();

   const UTATLootSettings& lootSettings = UTATLootSettings::Get();

   auto addLootEntries = [&lootSettings, this](TConstArrayView<FTATLootIdentifier> loot) {
      for (const FTATLootIdentifier& lootIdentifier : loot)
      {
         check(lootIdentifier.IsValid());
         const FTATLootInfo* lootInfo = lootSettings.GetLootInfo(this, lootIdentifier);
         check(lootInfo);

         UListView* lootListView = GetListViewForLootType(lootInfo->LootType);
         check(IsValid(lootListView));
         _lootEntryMgr.ConstructListEntry(lootIdentifier, lootListView);
      }
   };

   // Populate stashed loot items
   addLootEntries(matchPersistentData.CarriedLoot);
   addLootEntries(matchPersistentData.AllyCarriedLoot);
   addLootEntries(matchPersistentData.StashedLoot);

   // Populate total loot values
   if (matchPersistentData.CompletionState == EMatchCompletionState::Escaped)
   {
      // NB: Convert from 0-indexed order placement in C++ to a more-ergonomic one-indexed value in BP
      SetEscapeOrderPlacement(true, matchPersistentData.EscapeOrderPlacement + 1);
   }
   else
   {
      SetEscapeOrderPlacement(false, -1);
   }

   // Compute the max loot, so we can mark the leaders
   int32 maxLootValue = 0;

   TArray<FTATPostMatchScoreboardEntryData> scoreboardEntries;
   scoreboardEntries.Reserve(playerRankingData.Num());
   for (const FMatchPersistentRankingPlayerData& rankingData : playerRankingData)
   {
      FTATPostMatchScoreboardEntryData scoreboardEntry;
      scoreboardEntry.RankingData = rankingData;

      scoreboardEntries.Add(scoreboardEntry);

      maxLootValue = FMath::Max(maxLootValue, rankingData.TotalLootValue);
   }

   // Mark the loot leaders (including ties)
   for (FTATPostMatchScoreboardEntryData& scoreboardEntry : scoreboardEntries)
   {
      scoreboardEntry.IsLootLeader = scoreboardEntry.RankingData.TotalLootValue >= maxLootValue;
   }

   // Sort the ranking data by most - least loot
   scoreboardEntries.Sort([](const FTATPostMatchScoreboardEntryData& lhs, const FTATPostMatchScoreboardEntryData& rhs)
   {
      return lhs.RankingData.TotalLootValue > rhs.RankingData.TotalLootValue;
   });
   SetPlayerLootRankingData(scoreboardEntries);
}
