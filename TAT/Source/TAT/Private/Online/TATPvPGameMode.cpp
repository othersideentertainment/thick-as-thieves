// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Online/TATPvPGameMode.h"

// tat
#include "Character/TATTeams.h"
#include "Developer/TATProjectSettings.h"

// ue
#include "Engine/AssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPvPGameMode)

ATATPvPGameMode::ATATPvPGameMode()
   : Super()
{
   // default
   _nextTeamAssignment = UTATProjectSettings::GetTeamAssignmentForCharacterType(ETATTeamCharacterType::Player);
}


uint8 ATATPvPGameMode::_GetTeamIndexForNewPlayer(APlayerController* newPlayer)
{
   return ++_nextTeamAssignment;
}
