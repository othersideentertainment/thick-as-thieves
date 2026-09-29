// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose dedicated server
#include "OSEDedicatedServerManagerGameliftTypes.h"
#include "ServerManager/OSEDedicatedServerManagerBase.h"

// gamelift
#include "GameLiftServerSDK.h"

#include "OSEDedicatedServerManagerGameLift.generated.h"

UCLASS()
class OSEDEDICATEDSERVERGAMELIFT_API UOSEDedicatedServerManagerGameLift : public UOSEDedicatedServerManagerBase
{
   GENERATED_BODY()

public:
   // lifecycle
   virtual void Init(UGameInstance* gameInstance) override;
   virtual void Shutdown() override;
   virtual void ServerReadyToAcceptPlayers() override;

   // login
   virtual void PreLogin(const FString& options, const FString& address, const FUniqueNetIdRepl& uniqueId, FString& errorMessage) override;
   virtual void Login(const FString& options, const FUniqueNetIdRepl& uniqueId, FString& errorMessage) override;
   virtual void Logout(AController* exitingController) override;

   virtual void Tick(float deltaTime) override;

private:
   void _UpdateServerStateFromGameSession(Aws::GameLift::Server::Model::GameSession gameSession);
   void _GameliftProcessReady();
   void _GameliftActivateGameSession();
   void _GameliftProcessEnding();
   bool _GameliftAcceptPlayerSession(const FString& playerSessionId);
   bool _GameliftRemovePlayerSession(const FString& playerSessionId);
   void _TickTimeSincePlayerSeen(float deltaTime);

private:
   FGameLiftServerSDKModule* _gameLiftSdkModule = nullptr;
   FOSEDedicatedServerGameliftState _gameLiftState;
   EOSEGameliftServerState _state = EOSEGameliftServerState::None;
   bool _isQueuedForBackfill = false;
   bool _isReadyForPlayers = false;
   float _timeSinceGameSessionActivation = 0.0f;

   UPROPERTY(Transient)
   TMap<FUniqueNetIdRepl, FOSEDedicatedServerGameliftPlayerInfo> _playerInfo;
};
