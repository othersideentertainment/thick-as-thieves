// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEDedicatedServerSubsystem.h"

// ose dedicated server
#include "OSEDedicatedServerSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEDedicatedServerSubsystem)

void UOSEDedicatedServerSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);

   UGameInstance* gameInstance = GetGameInstance();
   check(gameInstance);

   if (gameInstance->IsDedicatedServerInstance())
   {
      const UOSEDedicatedServerSettings& settings = UOSEDedicatedServerSettings::Get();

#if WITH_EDITOR
      // all editor variants get a development server, this supports running the editor in -server mode as well as PIE as a client
      check(settings.DevelopmentServerManagerClass.IsValid());
      _serverMgr = NewObject<UOSEDedicatedServerManagerBase>(this, settings.DevelopmentServerManagerClass.LoadSynchronous());
#else
      const bool forceDevServer = FParse::Param(FCommandLine::Get(), TEXT("force_dev_server"));
      if (forceDevServer)
      {
         // optionally force a dev server
         check(settings.DevelopmentServerManagerClass.IsValid());
         _serverMgr = NewObject<UOSEDedicatedServerManagerBase>(this, settings.DevelopmentServerManagerClass.LoadSynchronous());
      }
      else
      {
         // non editor configs get a real dedicated server, which can be installed via settings.  current impl is going to be a gamelift
         // server but we could replace this in the future with whatever we need to
         check(settings.ServerManagerClass.IsValid());
         _serverMgr = NewObject<UOSEDedicatedServerManagerBase>(this, settings.ServerManagerClass.LoadSynchronous());
      }
#endif // WITH_EDITOR
   
      // should have made something!
      check(_serverMgr);
      _serverMgr->Init(gameInstance);
   }
}

void UOSEDedicatedServerSubsystem::Deinitialize()
{
   Super::Deinitialize();
}

