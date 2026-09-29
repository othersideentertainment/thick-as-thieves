// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Loot/TATLootSubsystem.h"

// tat
#include "Developer/TATLootSettings.h"
#include "Loot/TATLootInventory.h"
#include "Online/TATGameState.h"
#include "Player/TATPlayerState.h"

// ue
#include "Engine/DataTable.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLootSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogTATLootSubsystem, Log, All);


const UTATLootSubsystem* UTATLootSubsystem::Get(const UObject* contextObj)
{
   check(IsValid(contextObj));

   UGameInstance* gameInstance = contextObj->GetWorld()->GetGameInstance();
   check(gameInstance);
   return gameInstance->GetSubsystem<UTATLootSubsystem>();
}

void UTATLootSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);

   // Force a sync-load of the loot table
   // TODO: investigate the Data Registry as a potential option for async load / caching support?
   const UTATLootSettings& lootSettings = UTATLootSettings::Get();
   _lootDataTable = lootSettings.LootDataTable.LoadSynchronous();
   if (IsValid(_lootDataTable))
   {
      _lootDataTableMap.Reset(_lootDataTable, [](const FTATLootInfo& lootInfo) { return lootInfo.LootIdentifier; });
   }
   else
   {
      UE_LOG(LogTATLootSubsystem, Warning, TEXT("Init() | LootDataTable is unassigned in TATLootSettings!"));
   }
}

// static
TArray<ATATPlayerState*> UTATLootSubsystem::GetPlayersRankedByStashedLoot(const UObject* contextObject)
{
   TArray<ATATPlayerState*> playersRanked;
   if (!IsValid(contextObject))
   {
      UE_LOG(LogTATLootSubsystem, Error, TEXT("GetTotalStashedLootPlayerRanking() called with invalid contextObject!"));
      return playersRanked;
   }

   const UWorld* world = contextObject->GetWorld();
   if (!world)
   {
      UE_LOG(LogTATLootSubsystem, Error, TEXT("GetTotalStashedLootPlayerRanking() | could not retrieve UWorld!"));
      return playersRanked;
   }

   // Should be present, but avoid a hard crash when it isn't
   const ATATGameState* gameState = world->GetGameState<ATATGameState>();
   if (!gameState)
   {
      UE_LOG(LogTATLootSubsystem, Error, TEXT("GetTotalStashedLootPlayerRanking() | could not retrieve ATATGameState!"));
      return playersRanked;
   }

   playersRanked = gameState->GetTATPlayerStates();
   playersRanked.Sort([](const ATATPlayerState& psLeft, const ATATPlayerState& psRight)
      {
         const UTATLootInventoryComponent* lootInventoryleft = psLeft.GetLootInventoryComponent();
         const UTATLootInventoryComponent* lootInventoryRight = psRight.GetLootInventoryComponent();

         check(IsValid(lootInventoryleft));
         check(IsValid(lootInventoryRight));

         return lootInventoryleft->GetTotalStashedLootValue() > lootInventoryRight->GetTotalStashedLootValue();
      });

   return playersRanked;
}

int UTATLootSubsystem::GetStashedLootRankIndexForPlayer(const ATATPlayerState* playerState)
{
   if (!IsValid(playerState))
   {
      UE_LOG(LogTATLootSubsystem, Error, TEXT("GetStashedLootRankForPlayer() called with invalid playerState!"));
      return INDEX_NONE;
   }

   TArray<ATATPlayerState*> playersRanked = UTATLootSubsystem::GetPlayersRankedByStashedLoot(playerState);
   return playersRanked.IndexOfByPredicate([&](const ATATPlayerState* ps) { return ps == playerState; });
}

const UDataTable* UTATLootSubsystem::GetLootDataTable() const
{
   return _lootDataTable;
}

const FTATLootInfo* UTATLootSubsystem::FindLootInfo(const FTATLootIdentifier& lootId) const
{
   return _lootDataTableMap.Find(_lootDataTable, lootId);
}

const FTATLootInfo& UTATLootSubsystem::GetLootInfoChecked(const FTATLootIdentifier& lootId) const
{
   return _lootDataTableMap.GetRefChecked(_lootDataTable, lootId);
}
