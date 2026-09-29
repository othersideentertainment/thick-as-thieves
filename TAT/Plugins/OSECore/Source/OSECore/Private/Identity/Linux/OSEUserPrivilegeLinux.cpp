// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Identity/Linux/OSEUserPrivilegeLinux.h"

// ue4
#include "Online.h"
#include "Net/OnlineEngineInterface.h"

// ose
#include "Identity/OSEUserPrivilege.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEUserPrivilegeLinux)

/* static */
UOSEUserPrivilege* UOSEUserPrivilege::Create(UObject* outer)
{
   return NewObject<UOSEUserPrivilegeLinux>(outer);
}

void UOSEUserPrivilegeLinux::InitializePlatform()
{

}

void UOSEUserPrivilegeLinux::DeinitializePlatform()
{

}
