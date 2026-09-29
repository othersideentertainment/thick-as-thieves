// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

#include "TATPlayerQuestSlot.generated.h"

// The category of quest a player can have in a match
// They can have at most one per slot at a time
//
// (Can move this enum if there are users outside the PlayerState api)
UENUM(BlueprintType)
enum class ETATPlayerQuestSlot : uint8
{
   // The "main" quest shared by all players in the match
   // Still competitive(?), and tracked separately per player
   Mission,
   // Quests specific to a given player
   Contract,
   MAX                  UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(ETATPlayerQuestSlot, ETATPlayerQuestSlot::MAX);
