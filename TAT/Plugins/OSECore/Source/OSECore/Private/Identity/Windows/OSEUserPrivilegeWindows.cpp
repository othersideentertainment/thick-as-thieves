// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Identity/Windows/OSEUserPrivilegeWindows.h"

// ue4
#include "Online.h"
#include "Net/OnlineEngineInterface.h"

// ose
#include "Identity/OSEUserPrivilege.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEUserPrivilegeWindows)

/* static */
UOSEUserPrivilege* UOSEUserPrivilege::Create(UObject* outer)
{
   return NewObject<UOSEUserPrivilegeWindows>(outer);
}

void UOSEUserPrivilegeWindows::InitializePlatform()
{

}

void UOSEUserPrivilegeWindows::DeinitializePlatform()
{

}



