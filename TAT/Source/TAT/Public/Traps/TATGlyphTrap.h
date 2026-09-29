// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATTrapBase.h"

#include "TATGlyphTrap.generated.h"

class UTATOverlapTargetTriggerComponent;
enum class ETATOverlapTargetTriggerReason : uint8;

// A trap that (currently) activates on overlap with a stealth score threshold
//
// Naming this specifically, since this design could easily change
// Not to be confused with Glyph Indicators
UCLASS()
class TAT_API ATATGlyphTrap : public ATATTrapBase
{
   GENERATED_BODY()

public:
   ATATGlyphTrap();

protected:
   virtual void BeginPlay() override;
   virtual void _HandleStateChanged(const FTATTrapState& previousState) override;

private:
   void _AuthorityOnTargetFound(ETATOverlapTargetTriggerReason reason);

protected:
   UPROPERTY(EditDefaultsOnly)
   TObjectPtr<UTATOverlapTargetTriggerComponent> _overlapTargetTrigger;
};
