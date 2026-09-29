// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "GameFramework/TATMatchPersistentTypes.h"

// tat
#include "Loot/TATLootUtils.h"

// ue
#include "Player/TATPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATMatchPersistentTypes)

int FMatchPersistentData::CountAllLootOfType(const UObject* worldContext, ETATLootType type) const
{
   return UTATLootUtils::CountLootOfType(worldContext, CarriedLoot, type)
      + UTATLootUtils::CountLootOfType(worldContext, StashedLoot, type)
      + UTATLootUtils::CountLootOfType(worldContext, AllyCarriedLoot, type);
}

void FMatchPersistentData::Reset()
{
   *this = FMatchPersistentData();
}

FTATCachedPlayerInfo::FTATCachedPlayerInfo(const ATATPlayerState* player)
{
   check(player);
   PlayerName = player->GetPlayerName();
   CharacterType = player->GetTATCharacter();
}

void FMatchPersistentXPGainedData::Reset()
{
   *this = FMatchPersistentXPGainedData();
}

