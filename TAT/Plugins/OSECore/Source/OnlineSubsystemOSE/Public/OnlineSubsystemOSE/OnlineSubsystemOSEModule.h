// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class IOnlineFactory;

class FOnlineSubsystemOSEModule final : public IModuleInterface
{
public:
   // from IModuleInterface
   virtual void StartupModule() override;
   virtual void ShutdownModule() override;

private:
   IOnlineFactory* _onlineFactory = nullptr;
};
