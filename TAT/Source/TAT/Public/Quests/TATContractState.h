// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

#include "TATContractState.generated.h"

// An enum of the states that a quest can be in
// TODO: not super happy with naming here, but the only significant
//       ones so far are Unstarted and Complete, and those are fine
UENUM(BlueprintType)
enum class ETATContractState : uint8
{
   // Implicit default unstarted state
   Unstarted,
   // Needs to play narrative intro if explicit user action (skip if NA)
   // TBD Handwave-y
   Intro,
   // The meat of the quest state, where it is ready to be used in a match
   // NOTE: Not super happy with this name. Thought about Active, but already using
   //       that to mean active in a match, and don't want to confuse those.
   Objective,
   // Needs to play narrative outro if explicit user action (skip if NA)
   // TBD Handwave-y
   Outro,
   // Complete, and rewards granted
   Complete
};
