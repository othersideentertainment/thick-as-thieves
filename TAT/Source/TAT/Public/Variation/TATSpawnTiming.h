// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "TATSpawnTiming.generated.h"

// Timing at which the spawn is executed, but not the timing it was _chosen_ to spawn
UENUM()
enum class ETATSpawnTiming : uint8
{
   // Spawns at map start
   Initial,
   // Waits to spawn until the endgame happens a player did a quest objective (i.e. take the loot)
   EndgameQuest,
   // Waits to spawn until the endgame happens for any reason, including the timer
   EndgameAny
};

namespace SpawnTimingHelpers
{
   // could have used USOECommon, but :shrug: (or made the enum values be a bitmask directly, but I didn't really want to have a none)
   // Not hard to change later
   inline uint8 TimingToMask(ETATSpawnTiming timing)
   {
      return 1 << static_cast<uint8>(timing);
   }
}
