// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "GameFramework/OSEGameMode.h"

#include "TATHubGameMode.generated.h"

class APlayerStart;
class ATATSafeRoomPlayerStart;
class ATATToastBroadcaster;
struct FLinearColor;

UCLASS()
class TAT_API ATATHubGameMode : public AOSEGameMode
{
   GENERATED_BODY()

public:
   ATATHubGameMode();

   // from AGameModeBase
   virtual void StartPlay() override;
   virtual void BeginPlay() override;
   virtual bool AllowCheats(APlayerController* pc) override;
   virtual void PreLogin(const FString& options, const FString& address, const FUniqueNetIdRepl& uniqueId, FString& errorMessage) override;
   virtual APlayerController* Login(UPlayer* newPlayer, ENetRole inRemoteRole, const FString& portal, const FString& options, const FUniqueNetIdRepl& uniqueId, FString& errorMessage) override;
   virtual void PostLogin(APlayerController* newPlayer) override;
   virtual void Logout(AController* exitingController);
   virtual void GetSeamlessTravelActorList(bool toTransition, TArray<AActor*>& actorList) override;
   virtual void HandleSeamlessTravelPlayer(AController*& pc) override;

   // Returns the number of players who have connected to this server, but have not completed login
   int32 GetNumPlayersPendingLogin() const { return _pendingPlayerLogins.Num(); }

private:
   // Set of players that have started the login process but are not yet initialized
   TArray<FUniqueNetIdRepl, TInlineAllocator<6>> _pendingPlayerLogins;

};
