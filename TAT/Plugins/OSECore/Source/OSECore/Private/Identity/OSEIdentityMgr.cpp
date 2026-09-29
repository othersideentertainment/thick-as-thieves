// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// ose
#include "Identity/OSEIdentityMgr.h"
#include "Identity/OSEPlatformIdentity.h"
#include "Identity/OSEUserPrivilege.h"
#include "Identity/OSESaveGameSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEIdentityMgr)

void UOSEIdentityMgr::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);
   
   _platformIdentity = NewObject<UOSEPlatformIdentity>(this);
   _platformIdentity->Initialize(GetGameInstance());

   _userPrivilege = UOSEUserPrivilege::Create(this);
   // Check the privilege object was created (per-platform cpp defined)
   check(_userPrivilege);
   _userPrivilege->Initialize(GetGameInstance());

   _saveGameSystem = NewObject<UOSESaveGameSystem>(this);
   if (!_saveGameSystem->Initialize(GetGameInstance()))
   {
      _saveGameSystem = nullptr;
   }
}

void UOSEIdentityMgr::Deinitialize()
{
   Super::Deinitialize();
   
   if (_saveGameSystem)
   {
      _saveGameSystem->Deinitialize();
      _saveGameSystem = nullptr;
   }
   _userPrivilege->Deinitialize();
   _userPrivilege = nullptr;
   _platformIdentity->Deinitialize();
   _platformIdentity = nullptr;
}

