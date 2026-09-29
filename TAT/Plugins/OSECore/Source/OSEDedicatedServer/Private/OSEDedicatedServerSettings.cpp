// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEDedicatedServerSettings.h"

// ose dedicated server
#include "ServerManager/OSEDedicatedServerManagerDevelopment.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEDedicatedServerSettings)

UOSEDedicatedServerSettings::UOSEDedicatedServerSettings()
{
   ServerManagerClass = UOSEDedicatedServerManagerBase::StaticClass();
   DevelopmentServerManagerClass = UOSEDedicatedServerManagerDevelopment::StaticClass();
}

