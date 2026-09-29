// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OnlineSubsystemOSE/OnlineFactoryOSE.h"

// ose
#include "OnlineSubsystemOSE/OnlineSubsystemOSE.h"

IOnlineSubsystemPtr FOnlineFactoryOSE::CreateSubsystem(FName instanceName)
{
   TSharedPtr<FOnlineSubsystemOSE, ESPMode::ThreadSafe> subsystem = MakeShared<FOnlineSubsystemOSE, ESPMode::ThreadSafe>(instanceName);
   if (subsystem->IsEnabled())
   {
      if (!subsystem->Init())
      {
         subsystem->Shutdown();
         subsystem = nullptr;
      }
   }
   else
   {
      subsystem->Shutdown();
      subsystem = nullptr;
   }

   return subsystem;
}
