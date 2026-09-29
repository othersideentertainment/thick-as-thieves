// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#include "OSEPerceptionTest.h"

#define LOCTEXT_NAMESPACE "FOSEPerceptionTestModule"

void FOSEPerceptionTestModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FOSEPerceptionTestModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FOSEPerceptionTestModule, OSEPerceptionTest)
