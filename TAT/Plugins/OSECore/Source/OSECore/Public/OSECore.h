// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOSECore, Log, All);

class FOSECore : public IModuleInterface
{
public:

   /** IModuleInterface implementation */
   virtual void StartupModule() override;
   virtual void ShutdownModule() override;
   virtual bool IsGameModule() const override { return true; }

private:
#if WITH_EDITOR
   void _RegisterSettings();
   void _UnRegisterSettings();
   bool _HandleSettingsSaved();
   void _SetupCustomClassIcons();
#endif
};
