// (c) 2022-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "ServerManager/OSEDedicatedServerManagerBase.h"

#include "TATDedicatedServerManagerOnlineSession.generated.h"

UCLASS()
class TAT_API UTATDedicatedServerManagerOnlineSession : public UOSEDedicatedServerManagerBase
{
   GENERATED_BODY()

public:
   static UTATDedicatedServerManagerOnlineSession& GetChecked(UObject* worldContextObject);
   static UTATDedicatedServerManagerOnlineSession* Get(UObject* worldContextObject);

   //
   // lifecycle
   //
   virtual void Init(UGameInstance* gameInstance) override;
   virtual void Shutdown() override;

   // call when this server is loaded into the correct map and server-side configuration is complete and we're ready to accept players
   virtual void ServerReadyToAcceptPlayers() override;
   virtual void ServerEndMatch() override;

   // call these from a GameMode subclass
   virtual void PreLogin(const FString& options, const FString& address, const FUniqueNetIdRepl& uniqueId, FString& errorMessage) override;
   virtual void Login(const FString& options, const FUniqueNetIdRepl& uniqueId, FString& errorMessage) override;
   virtual void PostLogin(AController* newPlayer) override;
   virtual void Logout(AController* exitingController) override;

   // from FTickableObjectBase
   virtual void Tick(float deltaTime) override;

private:

   int32 _GetMaxPlayerCount() const;
   int32 _GetNumConnectedPlayers() const;

   UFUNCTION()
   void _OnMatchEndCleanup();

   FTimerHandle _matchEndCleanupTimer;
};
