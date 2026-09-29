// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "OnlineSubsystem.h"

class FOnlineFactoryOSE final : public IOnlineFactory
{
public:
   // from IOnlineFactory
   virtual IOnlineSubsystemPtr CreateSubsystem(FName InstanceName) override;
};
