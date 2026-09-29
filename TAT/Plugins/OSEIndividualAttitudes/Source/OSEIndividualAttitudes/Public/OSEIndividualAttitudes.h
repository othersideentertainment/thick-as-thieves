// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FOSEIndividualAttitudesModule : public IModuleInterface
{
public:

	// start IModuleInterface implementation
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	// end IModuleInterface implementation
};
