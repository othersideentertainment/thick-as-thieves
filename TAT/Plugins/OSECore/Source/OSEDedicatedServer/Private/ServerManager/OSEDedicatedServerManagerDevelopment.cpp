// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "ServerManager/OSEDedicatedServerManagerDevelopment.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEDedicatedServerManagerDevelopment)

// ue4

void UOSEDedicatedServerManagerDevelopment::Init(UGameInstance* gameInstance)
{
   Super::Init(gameInstance);
}

void UOSEDedicatedServerManagerDevelopment::Shutdown()
{
   Super::Shutdown();
   FGenericPlatformMisc::RequestExit(false);
}

