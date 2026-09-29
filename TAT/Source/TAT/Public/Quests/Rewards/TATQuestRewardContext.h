// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "SaveGame/TATCharacterSaveId.h"

// ue
#include "CoreMinimal.h"

class UTATSaveGame;

struct FTATQuestRewardContext
{
   TWeakObjectPtr<UTATSaveGame> SaveGame = nullptr;
   FTATCharacterSaveId Character;

   bool IsValid() const
   {
      return SaveGame.IsValid() && Character.IsValid();
   }
};
