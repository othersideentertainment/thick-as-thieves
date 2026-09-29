// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OnlineSubsystemOSE/OnlineSubsystemOSEModule.h"

// ose
#include "OnlineSubsystemOSE/OnlineSubsystemOSE.h"
#include "OnlineSubsystemOSE/OnlineFactoryOSE.h"

// ue4
#include "Modules/ModuleManager.h"
#include "OnlineSubsystemModule.h"
#include "OnlineSubsystem.h"

IMPLEMENT_GAME_MODULE(FOnlineSubsystemOSEModule, OnlineSubsystemOSE);

void FOnlineSubsystemOSEModule::StartupModule()
{
   _onlineFactory = new FOnlineFactoryOSE();
   FOnlineSubsystemModule& ossModule = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
   ossModule.RegisterPlatformService(FOnlineSubsystemOSE::sName, _onlineFactory);
}

void FOnlineSubsystemOSEModule::ShutdownModule()
{
   FOnlineSubsystemModule& ossModule = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
   ossModule.UnregisterPlatformService(FOnlineSubsystemOSE::sName);
   delete _onlineFactory;
   _onlineFactory = nullptr;
}
