// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OnlineSubsystemOSE/Windows/OSESteamNetDriver.h"

// ose
#include "OnlineSubsystemOSE/OnlineSubsystemOSE.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESteamNetDriver)

UOSESteamNetDriver::UOSESteamNetDriver(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
}

bool UOSESteamNetDriver::IsAvailable() const
{
   //
   // intentionally not calling Super here
   //

   // use the steam net driver if our online subsystem is using steam, otherwise say it's not available
   const FOnlineSubsystemOSE* subsystem = FOnlineSubsystemOSE::Get(this);
   if (subsystem != nullptr && subsystem->GetUnderlyingPlatformSubsystemType() == EOnlineSubsystemOSEPlatform::Steam)
   {
      // Net driver won't work if the online and socket subsystems don't exist
      ISocketSubsystem* steamSockets = ISocketSubsystem::Get(STEAM_SUBSYSTEM);
      if (steamSockets)
      {
         return true;
      }
   }
   return false;
}

