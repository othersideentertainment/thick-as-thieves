// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TATHearingStimSourceReactor.generated.h"

struct FGameplayTag;
// This class does not need to be modified.
UINTERFACE()
class UTATHearingStimSourceReactor : public UInterface
{
   GENERATED_BODY()
};

// A stim could have an instigator of a pawn, a player controller or a player state according to UTATAISense_Hearing::_IsInstigatorPlayer, so this
// interface will exist on each of those classes and pass down into the relevant components
class TAT_API ITATHearingStimSourceReactor
{
   GENERATED_BODY()

public:
   virtual void HandleReactToOwnStim(const FGameplayTag& stimTag, float loudness) = 0;
};
