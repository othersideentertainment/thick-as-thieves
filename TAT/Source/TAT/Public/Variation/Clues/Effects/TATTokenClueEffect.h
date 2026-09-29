// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATClueEffect.h"

#include "TATTokenClueEffect.generated.h"

class UTATInventoryToken;

// A clue effect that grants inventory tokens
USTRUCT(DisplayName="Grant Tokens")
struct FTATTokenClueEffect : public FTATClueEffect
{
   GENERATED_BODY()

   virtual void ApplyTo(APlayerState* playerState) const override;
#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void (const FText&)> reportError) const override;
#endif

   UPROPERTY(EditAnywhere)
   TArray<TObjectPtr<const UTATInventoryToken>> TokensToGrant;

   // Grant a token even if the player already has it
   UPROPERTY(EditAnywhere)
   bool AllowDuplicates = false;
};
