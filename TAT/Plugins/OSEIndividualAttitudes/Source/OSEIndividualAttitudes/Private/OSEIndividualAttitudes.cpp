// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "OSEIndividualAttitudes.h"

#define LOCTEXT_NAMESPACE "FOSEIndividualAttitudesModule"

void FOSEIndividualAttitudesModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FOSEIndividualAttitudesModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FOSEIndividualAttitudesModule, OSEIndividualAttitudes)
