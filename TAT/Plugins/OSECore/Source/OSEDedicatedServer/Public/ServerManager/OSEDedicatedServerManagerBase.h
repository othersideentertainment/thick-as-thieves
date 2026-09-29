// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

#include "OSEDedicatedServerManagerBase.generated.h"

class APlayerController;

UCLASS(Abstract)
class OSEDEDICATEDSERVER_API UOSEDedicatedServerManagerBase : public UObject, public FTickableGameObject
{
   GENERATED_BODY()

public:
   // static
   static UOSEDedicatedServerManagerBase& Get(UObject& worldContextObject);
   static UOSEDedicatedServerManagerBase* TryGet(UObject& worldContextObject);

   //
   // lifecycle
   //
   virtual void Init(UGameInstance* gameInstance);
   virtual void Shutdown();
   
   // call when this server is loaded into the correct map and server-side configuration is complete and we're ready to accept players
   virtual void ServerReadyToAcceptPlayers(); 
   virtual void ServerEndMatch();
   
   // call these from a GameMode subclass
   virtual void PreLogin(const FString& options, const FString& address, const FUniqueNetIdRepl& uniqueId, FString& errorMessage);
   virtual void Login(const FString& options, const FUniqueNetIdRepl& uniqueId, FString& errorMessage);
   virtual void PostLogin(AController* newPlayer);
   virtual void Logout(AController* exitingController);

   // from FTickableObjectBase
   virtual bool IsTickable() const override { return !HasAnyFlags(RF_ClassDefaultObject) && _gameInstance; }
   virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UOSEDedicatedServerManagerBase, STATGROUP_Tickables); }
   virtual UWorld* GetWorld() const override { return _gameInstance ? _gameInstance->GetWorld() : nullptr; }
   virtual void Tick(float deltaTime) override;   

   //
   // utl
   //
   int GetPort() const;
   FString GetIPAddress() const;

protected:
   UPROPERTY(Transient)
   TObjectPtr<UGameInstance> _gameInstance;
};

OSEDEDICATEDSERVER_API DECLARE_LOG_CATEGORY_EXTERN(LogOSEDedicatedServer, Log, All);
