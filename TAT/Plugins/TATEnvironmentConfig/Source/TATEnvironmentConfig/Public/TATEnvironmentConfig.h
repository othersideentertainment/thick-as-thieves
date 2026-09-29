// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "Modules/ModuleManager.h"

class FTATEnvironmentConfigModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

namespace TATEnvironmentConfig
{
   TATENVIRONMENTCONFIG_API const FString& GetEnvironmentName();
   TATENVIRONMENTCONFIG_API bool ShouldShowEnvironmentName();
}
