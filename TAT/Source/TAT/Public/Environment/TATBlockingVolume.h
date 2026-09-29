// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameFramework/Volume.h"

#include "TATBlockingVolume.generated.h"

// A tat-level almost-version of ABlockingVolume, so it can have the hotwire_notarget tag
//
// Could have also possibly been a blueprint, but I was less confident in doing a core redirect to one
//
// Not a subclass, since ABlockingVolume is lacking dll exports, and it didn't seem worth
UCLASS()
class TAT_API ATATBlockingVolume : public AVolume
{
   GENERATED_BODY()

public:
   // Sets default values for this actor's properties
   ATATBlockingVolume();
};
