// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

struct FTATCharacterSaveId;
struct FTATContractInfo;
class UTATSaveGame;

// Helpers for persistence of contract
// TODO: Rename?
namespace TATContractProgression
{
   void CompleteContract(UTATSaveGame* saveGame, FTATCharacterSaveId saveId, const FTATContractInfo& quest);
}
