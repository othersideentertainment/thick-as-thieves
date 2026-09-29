// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Modules/ModuleManager.h"
#include "SaveGameSystem.h"

class FTATAtomicSaveModule : public ISaveGameSystemModule
{
public:

   virtual void StartupModule() override;
   virtual void ShutdownModule() override;

   virtual ISaveGameSystem* GetSaveGameSystem() override;
};
