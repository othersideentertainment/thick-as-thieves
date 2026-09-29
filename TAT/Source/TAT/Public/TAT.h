// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FTAT : public IModuleInterface
{
public:
   virtual void StartupModule() override;
   virtual void ShutdownModule() override;
   virtual bool IsGameModule() const override
   {
      return true;
   }

private:
   FDelegateHandle _skeletalMeshLodDelegate;
};
