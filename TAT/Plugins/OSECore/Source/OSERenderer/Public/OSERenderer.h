// (c) 2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOSERenderer, Log, All);

class FOSERenderer : public IModuleInterface
{
public:

   // IModuleInterface implementation
   virtual void StartupModule() override;
   virtual void ShutdownModule() override;
   virtual bool IsGameModule() const override { return true; }
};
