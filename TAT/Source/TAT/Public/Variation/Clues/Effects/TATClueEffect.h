// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

#include "TATClueEffect.generated.h"

// A base class for something applied or granted to the player upon
// getting a clue.
//
// The initial use-case is granting tokens to the player after talking
// to an NPC cluegiver.
//
// TODO: Not super thrilled with name, but could not think of a better one.
//       May have a better idea as usage evolves. Also not sure how clue-specific
//       this actually is, but don't want to pre-emptively assume it is generic.
USTRUCT()
struct TAT_API FTATClueEffect
{
   GENERATED_BODY()

   virtual ~FTATClueEffect() = default;

   virtual void ApplyTo(APlayerState* playerState) const {}
#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void (const FText&)> reportError) const {}
#endif
};
