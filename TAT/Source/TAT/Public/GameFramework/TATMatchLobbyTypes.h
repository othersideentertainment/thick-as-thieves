// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "TATMatchLobbyTypes.generated.h"

UENUM()
enum class ETATMatchLobbyState : uint8
{
   None UMETA(Hidden),
   SetupMatch,
   CharacterSelect
};
