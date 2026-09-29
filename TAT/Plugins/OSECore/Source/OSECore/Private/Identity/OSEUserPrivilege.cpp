// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Identity/OSEUserPrivilege.h"


// ose
#include "Identity/OSEIdentityMgr.h"
#include "Identity/OSEPlatformIdentity.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEUserPrivilege)

DEFINE_LOG_CATEGORY_STATIC(LogOSEUserPrivilege, Log, All);

/* static */
const UOSEUserPrivilege& UOSEUserPrivilege::Get(const UObject& contextObject)
{
   return GetRef(contextObject);
}

/* static */
UOSEUserPrivilege& UOSEUserPrivilege::GetRef(const UObject& contextObject)
{
   UWorld* world = contextObject.GetWorld();
   check(world);
   UGameInstance* gameInstance = world->GetGameInstance();
   UOSEIdentityMgr* identityMgr = gameInstance->GetSubsystem<UOSEIdentityMgr>();
   check(identityMgr);
   UOSEUserPrivilege* userPrivilege = identityMgr->GetUserPrivilege();
   check(userPrivilege);
   return *userPrivilege;
}

void UOSEUserPrivilege::Initialize(UGameInstance* owningGameInstance)
{
   check(owningGameInstance);
   _gameInstance = owningGameInstance;

   _privilegeTypes = {
      EUserPrivileges::CanCommunicateOnline,
      EUserPrivileges::CanPlay,
      EUserPrivileges::CanPlayOnline,
      EUserPrivileges::CanUserCrossPlay,
      EUserPrivileges::CanUseUserGeneratedContent,
   };

   InitializePlatform();
}

void UOSEUserPrivilege::Deinitialize()
{
   DeinitializePlatform();
}


//--------------------------------------------------------------------------
// State machine (has checked privilege)

void UOSEUserPrivilege::_SetPrivilegeState(EOSEUserPrivilegeState newState)
{
   // Alreay in state?
   if (newState == _knowsPrivilegesState)
   {
      return;
   }

   _knowsPrivilegesState = newState;
   OnPrivilegeStateChanged.Broadcast(newState);
}


//--------------------------------------------------------------------------
// Public methods

bool UOSEUserPrivilege::HasPrivilege(EUserPrivileges::Type privilege) const
{
   // User privileges not yet known? Return false...
   if (_knowsPrivilegesState != EOSEUserPrivilegeState::Known)
   {
      UE_LOG(LogOSEUserPrivilege, Warning, TEXT("HasPrivilege(type): User privileges are not currently known!"));
      return false;
   }
   return _userKnownHasPrivFlags & (1U << static_cast<int>(privilege));
}

bool UOSEUserPrivilege::CheckPrivileges()
{
   UE_LOG(LogOSEUserPrivilege, Verbose, TEXT("CheckPrivileges"));

   // Return if currently checking (allow states 'unknown' and 'known' for re-check)
   if (_knowsPrivilegesState == EOSEUserPrivilegeState::Checking)
   {
      return false;
   }

   // Get user's netId (from platform identity)
   UOSEIdentityMgr* identityMgr = _gameInstance->GetSubsystem<UOSEIdentityMgr>();
   check(identityMgr);
   FUniqueNetIdPtr userNetId = identityMgr->GetPlatformIdentity()->GetUniqueNetId();
   check(userNetId.IsValid());

   _SetPrivilegeState(EOSEUserPrivilegeState::Checking);
   _userKnownHasPrivFlags = 0U;

   // Check privileges async (callbacks `_OnPrivilegeCheckCompleteCallback`)
   IOnlineIdentityPtr identity = Online::GetIdentityInterface();
   check(identity);
   _outstandingAsyncTasks = _privilegeTypes.Num();
   for (EUserPrivileges::Type e : _privilegeTypes)
   {
      identity->GetUserPrivilege(*userNetId, e, IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateUObject(this, &UOSEUserPrivilege::_OnPrivilegeCheckCompleteCallback));
   }
   return true;
}

bool UOSEUserPrivilege::RefreshPrivileges()
{
   UE_LOG(LogOSEUserPrivilege, Verbose, TEXT("RefreshPrivileges"));
   return CheckPrivileges();
}


//--------------------------------------------------------------------------
// Async check callbacks

void UOSEUserPrivilege::_OnPrivilegeCheckCompleteCallback(const FUniqueNetId& localUserId, EUserPrivileges::Type privilege, uint32 privilegeResult)
{
   UE_LOG(LogOSEUserPrivilege, Verbose, TEXT("_OnPrivilegeCheckCompleteCallback: privilege=%i, privResult=%i"), privilege, privilegeResult);

   // Has privilege?
   const uint32 successMask = static_cast<uint32>(IOnlineIdentity::EPrivilegeResults::NoFailures);
   if (privilegeResult == successMask)
   {
      _userKnownHasPrivFlags |= 1U << static_cast<int>(privilege);
   }

   // Check expecting remaining async tasks, in case we receive
   // more async priv check callbacks than `_privilegeTypes.Num()`.
   checkf(_outstandingAsyncTasks > 0, TEXT("_OnPrivilegeCheckCompleteCallback: More callbacks than expected (remaining: %i)!"), _outstandingAsyncTasks);

   // Last outstanding async-callback?
   if (--_outstandingAsyncTasks == 0)
   {
      _SetPrivilegeState(EOSEUserPrivilegeState::Known);
   }
}

