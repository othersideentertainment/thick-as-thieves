// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FOSEDedicatedServer : public IModuleInterface
{
public:

   // IModuleInterface implementation
   virtual void StartupModule() override;
   virtual void ShutdownModule() override;
   virtual bool IsGameModule() const override { return true; }
};
