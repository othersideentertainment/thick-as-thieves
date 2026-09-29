// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FOSEPerceptionModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
