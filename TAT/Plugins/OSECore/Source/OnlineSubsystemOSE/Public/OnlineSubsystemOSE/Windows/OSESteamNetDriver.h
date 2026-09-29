// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ue4
#include "SteamNetDriver.h"

#include "OSESteamNetDriver.generated.h"

UCLASS(transient, config = Engine)
class UOSESteamNetDriver : public USteamNetDriver
{
   GENERATED_UCLASS_BODY()

public:
   // from USteamNetDriver
   virtual bool IsAvailable() const override;
};
