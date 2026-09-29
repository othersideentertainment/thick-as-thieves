// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OnlineSubsystemOSE/OnlineSubsystemOSE.h"

// ose core
#include "OSECore/Public/OSECommon.h"

// ue4
#include "OnlineSubsystem.h"
#include "OnlineSubsystemNames.h"
#include "OnlineSubsystemSteam.h"
#include "OnlineSubsystemUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlineSubsystemOSE)

const FName FOnlineSubsystemOSE::sName = TEXT("OSE");

// defines interface functions that we don't implement at the ose level
#define RETURN_UNDERLYING_PLATFORM_IMPL(FuncName) \
{ \
   if (IOnlineSubsystem* subSystem = GetUnderlyingPlatformSubsystem()) \
   { \
      return subSystem->FuncName(); \
   } \
   return nullptr; \
}

DEFINE_LOG_CATEGORY(LogOnlineSubsystemOSE);

FOnlineSubsystemOSE::FOnlineSubsystemOSE(FName instanceName)
   : FOnlineSubsystemImpl(FOnlineSubsystemOSE::sName, instanceName)
{

}

/* static */
FOnlineSubsystemOSE* FOnlineSubsystemOSE::Get(const UObject* contextObj)
{
   if (contextObj != nullptr)
   {
      return static_cast<FOnlineSubsystemOSE*>(Online::GetSubsystem(contextObj->GetWorld(), FOnlineSubsystemOSE::sName));
   }
   return nullptr;
}

bool FOnlineSubsystemOSE::Init()
{   
   // this getter will also cause our subsystem init!
   IOnlineSubsystem* platformOSS = GetUnderlyingPlatformSubsystem();
   check(platformOSS);
   UE_LOG(LogOnlineSubsystemOSE, Log, TEXT("Using platform subsystem \"%s\""), *platformOSS->GetSubsystemName().ToString());

   return true;
}

bool FOnlineSubsystemOSE::Shutdown()
{
   FOnlineSubsystemImpl::Shutdown();

   return true;
}

FString FOnlineSubsystemOSE::GetAppId() const
{
   return TEXT("AppId_OSE");
}

FText FOnlineSubsystemOSE::GetOnlineServiceName() const
{
   return INVTEXT("OSE");
}

FName FOnlineSubsystemOSE::GetSubsystemName() const
{
   // for now we're going to lie and pretend we're the underlying online subsystem.
   // but once we implement identity we'll want to flip this back over to "OSE"
   if (IOnlineSubsystem* subSystem = GetUnderlyingPlatformSubsystem())
   {
      return subSystem->GetSubsystemName();
   }
   return sName;
}

IOnlineSessionPtr FOnlineSubsystemOSE::GetSessionInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetSessionInterface)
IOnlineFriendsPtr FOnlineSubsystemOSE::GetFriendsInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetFriendsInterface)
IOnlinePartyPtr FOnlineSubsystemOSE::GetPartyInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetPartyInterface)
IOnlineGroupsPtr FOnlineSubsystemOSE::GetGroupsInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetGroupsInterface)
IOnlineSharedCloudPtr FOnlineSubsystemOSE::GetSharedCloudInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetSharedCloudInterface)
IOnlineUserCloudPtr FOnlineSubsystemOSE::GetUserCloudInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetUserCloudInterface)
IOnlineEntitlementsPtr FOnlineSubsystemOSE::GetEntitlementsInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetEntitlementsInterface)
IOnlineLeaderboardsPtr FOnlineSubsystemOSE::GetLeaderboardsInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetLeaderboardsInterface)
IOnlineVoicePtr FOnlineSubsystemOSE::GetVoiceInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetVoiceInterface)
IOnlineExternalUIPtr FOnlineSubsystemOSE::GetExternalUIInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetExternalUIInterface)
IOnlineTimePtr FOnlineSubsystemOSE::GetTimeInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetTimeInterface)
IOnlineIdentityPtr FOnlineSubsystemOSE::GetIdentityInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetIdentityInterface)
IOnlineTitleFilePtr FOnlineSubsystemOSE::GetTitleFileInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetTitleFileInterface)
//IOnlineStorePtr FOnlineSubsystemOSE::GetStoreInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetStoreInterface) // DEPRECATED
IOnlineStoreV2Ptr FOnlineSubsystemOSE::GetStoreV2Interface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetStoreV2Interface)
IOnlinePurchasePtr FOnlineSubsystemOSE::GetPurchaseInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetPurchaseInterface)
IOnlineEventsPtr FOnlineSubsystemOSE::GetEventsInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetEventsInterface)
IOnlineAchievementsPtr FOnlineSubsystemOSE::GetAchievementsInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetAchievementsInterface)
IOnlineSharingPtr FOnlineSubsystemOSE::GetSharingInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetSharingInterface)
IOnlineUserPtr FOnlineSubsystemOSE::GetUserInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetUserInterface)
IOnlineMessagePtr FOnlineSubsystemOSE::GetMessageInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetMessageInterface)
IOnlinePresencePtr FOnlineSubsystemOSE::GetPresenceInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetPresenceInterface)
IOnlineChatPtr FOnlineSubsystemOSE::GetChatInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetChatInterface)
IOnlineStatsPtr FOnlineSubsystemOSE::GetStatsInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetStatsInterface)
IOnlineTurnBasedPtr FOnlineSubsystemOSE::GetTurnBasedInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetTurnBasedInterface)
IOnlineTournamentPtr FOnlineSubsystemOSE::GetTournamentInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetTournamentInterface)
IOnlineGameActivityPtr FOnlineSubsystemOSE::GetGameActivityInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetGameActivityInterface)
IOnlineGameItemStatsPtr FOnlineSubsystemOSE::GetGameItemStatsInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetGameItemStatsInterface)
IOnlineGameMatchesPtr FOnlineSubsystemOSE::GetGameMatchesInterface() const RETURN_UNDERLYING_PLATFORM_IMPL(GetGameMatchesInterface)

bool FOnlineSubsystemOSE::Tick(float deltaTime)
{
   if (!FOnlineSubsystemImpl::Tick(deltaTime))
   {
      return false;
   }

   return true;
}

IOnlineSubsystem* FOnlineSubsystemOSE::GetUnderlyingPlatformSubsystem() const
{
   if (_cachedPlatformOnlineSubsystem)
      return _cachedPlatformOnlineSubsystem;

   UWorld* world = GetWorldForOnline(InstanceName);

   // which platform would we *like* to initialize?
   EOnlineSubsystemOSEPlatform targetPlatformInit = EOnlineSubsystemOSEPlatform::MAX;

   // we can fail to init online subsystems if we don't have entitlements for this game on the platform (like Steam)
#if PLATFORM_MAC || PLATFORM_LINUX || PLATFORM_WINDOWS
   if (GEngine->IsEditor())
   {
      // in the editor we run ip games
      targetPlatformInit = EOnlineSubsystemOSEPlatform::Null;
   }
   else
   {
      // NOTE: Can't use gameInstance->IsDedicatedServerInstance() here because world may be null when actually initializing dedicated servers
      if (IsRunningDedicatedServer())
      {
         // TODO: If we're hosting Steam dedicated servers only we'll want to change this to Steam
         targetPlatformInit = EOnlineSubsystemOSEPlatform::Null;
      }
      else
      {
         // running -game or cooked game builds gets you steam (editor case is already handled up above)
         targetPlatformInit = EOnlineSubsystemOSEPlatform::Steam;
      }
   }

   // pc cheats!
   
   // cmdline opt to override default behavior and force steam, works in editor, dedicated server, package, uncooked etc
   const bool forceSteamInPCBuilds = FParse::Param(FCommandLine::Get(), TEXT("force_steam"));
   if (forceSteamInPCBuilds)
   {
      UE_LOG(LogOnlineSubsystemOSE, Log, TEXT("Attempting to force using steam in editor builds..."));
      targetPlatformInit = EOnlineSubsystemOSEPlatform::Steam;
   }

   if (targetPlatformInit == EOnlineSubsystemOSEPlatform::Steam)
   {
      // cmdline opt to override default behavior and disable steam
      const bool noSteamInPCBuilds = FParse::Param(FCommandLine::Get(), TEXT("no_steam")) || FParse::Param(FCommandLine::Get(), TEXT("nosteam"));
      if (noSteamInPCBuilds)
      {
         UE_LOG(LogOnlineSubsystemOSE, Log, TEXT("Would have used Steam, but no_steam/nosteam was passed, so we're using the null online subsystem"));
         targetPlatformInit = EOnlineSubsystemOSEPlatform::Null;
      }
   }
#elif PLATFORM_XSX
   targetPlatformInit = EOnlineSubsystemOSEPlatform::XSX;
#elif PLATFORM_PS5
   targetPlatformInit = EOnlineSubsystemOSEPlatform::PS5;
#endif

   // When building for a platform, other subsystem platform OSS names
   // may not be defined (if not, define as empty FName).
#ifndef PS5_SUBSYSTEM
#define PS5_SUBSYSTEM FName()
#endif
#ifndef XSX_SUBSYSTEM
#define XSX_SUBSYSTEM FName()
#endif

   // ok now init it!
   IOnlineSubsystem* subSystem = nullptr;
   switch (targetPlatformInit)
   {
   case EOnlineSubsystemOSEPlatform::Null:
      {
         subSystem = _GetNullSubsystem();
      }
      break;
   case EOnlineSubsystemOSEPlatform::Steam:
      {
         subSystem = Online::GetSubsystem(world, STEAM_SUBSYSTEM);
      }
      break;
   case EOnlineSubsystemOSEPlatform::XSX:
      {
         subSystem = Online::GetSubsystem(world, XSX_SUBSYSTEM);
      }
      break;

   case EOnlineSubsystemOSEPlatform::PS5:
      {
         subSystem = Online::GetSubsystem(world, PS5_SUBSYSTEM);
      }
      break;

   default:
      checkNoEntry();
      break;
   }



   // fall back to null if we couldn't init anything
   if (!subSystem)
   {
      subSystem = _GetNullSubsystem();
   }

   // cache it off to avoid more expensive GetSubsystem() calls in the future
   const_cast<FOnlineSubsystemOSE*>(this)->_cachedPlatformOnlineSubsystem = subSystem;

   return subSystem;
}

EOnlineSubsystemOSEPlatform FOnlineSubsystemOSE::GetUnderlyingPlatformSubsystemType() const
{
   if (IOnlineSubsystem* subSystem = GetUnderlyingPlatformSubsystem())
   {
      FName subSystemName = subSystem->GetSubsystemName();
      if (subSystemName == NULL_SUBSYSTEM)
         return EOnlineSubsystemOSEPlatform::Null;
      else if (subSystemName == STEAM_SUBSYSTEM)
         return EOnlineSubsystemOSEPlatform::Steam;
   }
   return EOnlineSubsystemOSEPlatform::MAX;
}

IOnlineSubsystem* FOnlineSubsystemOSE::_GetNullSubsystem() const
{
   UWorld* world = GetWorldForOnline(InstanceName);
   return Online::GetSubsystem(world, NULL_SUBSYSTEM);
}

