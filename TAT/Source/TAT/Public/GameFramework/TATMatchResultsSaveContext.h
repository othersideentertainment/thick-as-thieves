// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

struct FMatchPersistentData;
struct FMatchPersistentAllyUpdate;
class UTATSaveGame;
struct FMatchPersistentXPGainedData;

namespace TATMatchResultsSaveContext
{
   void Apply(const FMatchPersistentData& results, UTATSaveGame* saveGame);

   FMatchPersistentXPGainedData ApplyXP(const FMatchPersistentData& results, UTATSaveGame* saveGame);
};
