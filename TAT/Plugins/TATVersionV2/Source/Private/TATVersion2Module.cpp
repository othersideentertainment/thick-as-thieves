// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "TATVersion2Module.h"

#include "TATVersionV2.h"

#define LOCTEXT_NAMESPACE "FTATVersion2Module"

void FTATVersion2Module::StartupModule()
{
   UTATVersionV2::CacheBuildArtifactDescriptors();
   UTATVersionV2::CacheBuildArtifactDescriptor();
   auto descriptor = UTATVersionV2::GetBuildArtifactDescriptor();
   UE_LOG(LogInit, Log, TEXT("Build Artifact Descriptor: %s"), *descriptor);
}

void FTATVersion2Module::ShutdownModule()
{
   // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
   // we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTATVersion2Module, TATVersionV2)
