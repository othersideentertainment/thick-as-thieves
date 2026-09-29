// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "ServerManager/OSEDedicatedServerManagerBase.h"

// ose dedicated server
#include "OSEDedicatedServerSettings.h"
#include "OSEDedicatedServerSubsystem.h"

// ue4
#include "SocketSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/NetworkVersion.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEDedicatedServerManagerBase)

DEFINE_LOG_CATEGORY(LogOSEDedicatedServer);

/* static */
UOSEDedicatedServerManagerBase& UOSEDedicatedServerManagerBase::Get(UObject& worldContextObject)
{
   UGameInstance* gameInstance = worldContextObject.GetWorld()->GetGameInstance();
   check(gameInstance);
   UOSEDedicatedServerSubsystem* serverSubsystem = gameInstance->GetSubsystem<UOSEDedicatedServerSubsystem>();
   check(serverSubsystem);
   UOSEDedicatedServerManagerBase* serverMgr = serverSubsystem->GetDedicatedServerMgr();
   check(serverMgr);
   return *serverMgr;
}

UOSEDedicatedServerManagerBase* UOSEDedicatedServerManagerBase::TryGet(UObject& worldContextObject)
{
   UGameInstance* gameInstance = worldContextObject.GetWorld()->GetGameInstance();
   check(gameInstance);
   UOSEDedicatedServerSubsystem* serverSubsystem = gameInstance->GetSubsystem<UOSEDedicatedServerSubsystem>();
   if (serverSubsystem == nullptr)
   {
      return nullptr;
   }
   return serverSubsystem->GetDedicatedServerMgr();
}

void UOSEDedicatedServerManagerBase::Init(UGameInstance* gameInstance)
{
   check(gameInstance);
   _gameInstance = gameInstance;

   // TODO: Server reboot when we're idle too long so we don't get huge world times (breaks a lot of things!)
}

void UOSEDedicatedServerManagerBase::Shutdown()
{
   SetTickableTickType(ETickableTickType::Never);
}

void UOSEDedicatedServerManagerBase::ServerReadyToAcceptPlayers()
{

}

void UOSEDedicatedServerManagerBase::ServerEndMatch()
{
}

void UOSEDedicatedServerManagerBase::PreLogin(const FString& options, const FString& address, const FUniqueNetIdRepl& uniqueId, FString& errorMessage)
{
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("PreLogin %s"), uniqueId.IsValid() ? *uniqueId->ToString() : TEXT("None"));
}

void UOSEDedicatedServerManagerBase::Login(const FString& options, const FUniqueNetIdRepl& uniqueId, FString& errorMessage)
{
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("Login %s"), uniqueId.IsValid() ? *uniqueId->ToString() : TEXT("None"));
}

void UOSEDedicatedServerManagerBase::PostLogin(AController* newPlayer)
{
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("PostLogin %s"), newPlayer ? *newPlayer->GetName() : TEXT("None"));
}

void UOSEDedicatedServerManagerBase::Logout(AController* exitingController)
{
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("Logout %s"), exitingController ? *exitingController->GetName() : TEXT("None"));
}

void UOSEDedicatedServerManagerBase::Tick(float deltaTime)
{

}

int UOSEDedicatedServerManagerBase::GetPort() const
{
   int32 serverPort = INDEX_NONE;

   // -Port gets passed into us
   if (!FParse::Value(FCommandLine::Get(), TEXT("Port="), serverPort))
   {
      // otherwise use the default engine port
      const int32 defaultPort = FURL::UrlConfig.DefaultPort;
      serverPort = defaultPort;
   }

   check(serverPort != INDEX_NONE);
   return serverPort;
}

FString UOSEDedicatedServerManagerBase::GetIPAddress() const
{
   ISocketSubsystem* socketSys = ISocketSubsystem::Get();
   check(socketSys);
   bool canBindAll = false;
   TSharedPtr<FInternetAddr> address = socketSys->GetLocalHostAddr(*GLog, canBindAll);
   if (address)
   {
      static const bool kAppendPort = false;
      return address->ToString(kAppendPort);
   }
   return FString();
}

