// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// gamelift
#include "GameLiftServerSDK.h"

#include "OSEDedicatedServerManagerGameliftTypes.generated.h"

enum EOSEGameliftServerState
{
   None, // Init() hasn't been called yet
   WaitingForGameSession, // waiting on Gamelift to give us a game session to configure ourselves for
   WaitingForServerToAcceptPlayers, // waiting for game code to call ServerReadyToAcceptPlayers() which will activate us for players to start joining
   InGameSession, // we are running a game session
   Terminating, // we are ending our game session / instance
};

USTRUCT()
struct FOSEDedicatedServerGameliftPlayerInfo
{
   GENERATED_BODY()

public:
   FString PlayerSessionId;
   float TimeSinceSeen = 0.0f;
};

struct FGameliftMatchmakerData
{
   FString MatchId;
   FString MatchmakingConfigurationArn;
   FString MatchmakingConfiguration;
   FString MatchmakingRegion;

   void FillFrom(const FString& matchmakerJSONPayloadStr);
   void DumpLog();
};

struct FOSEDedicatedServerGameliftState
{
   // copy of the game session
   Aws::GameLift::Server::Model::GameSession GameSession;

   // info we parse out from the game session
   FString GameSessionArn;
   FGameliftMatchmakerData MatchmakerData;
   FString BackfillTicketId;

   void FillFrom(const Aws::GameLift::Server::Model::GameSession& gameSession);
   void DumpLog();
};
