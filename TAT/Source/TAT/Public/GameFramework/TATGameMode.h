// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "GameFramework/OSEGameMode.h"

#include "TATGameMode.generated.h"

class APlayerStart;
class ATATSafeRoomPlayerStart;
class ATATToastBroadcaster;
struct FLinearColor;

UCLASS()
class TAT_API ATATGameMode : public AOSEGameMode
{
   GENERATED_BODY()

public:
   ATATGameMode();

   // from AGameModeBase
   virtual void StartPlay() override;
   virtual void BeginPlay() override;
   virtual void PostInitializeComponents() override;
   virtual bool AllowCheats(APlayerController* pc) override;
   virtual void PreLogin(const FString& options, const FString& address, const FUniqueNetIdRepl& uniqueId, FString& errorMessage) override;
   virtual APlayerController* Login(UPlayer* newPlayer, ENetRole inRemoteRole, const FString& portal, const FString& options, const FUniqueNetIdRepl& uniqueId, FString& errorMessage) override;
   void _TryAssignTeam(APlayerController* newPlayer);
   virtual void PostLogin(APlayerController* newPlayer) override;
   virtual void Logout(AController* exitingController);
   virtual void GetSeamlessTravelActorList(bool toTransition, TArray<AActor*>& actorList) override;
   virtual void HandleSeamlessTravelPlayer(AController*& pc) override;
   virtual AActor* ChoosePlayerStart_Implementation(AController* controller) override;
   virtual bool UpdatePlayerStartSpot(AController* player, const FString& portal, FString& outErrorMessage);

   FLinearColor GetColorForTeamID(const uint8 teamID);

   ATATToastBroadcaster* GetToastBroadcaster() const { return _toastBroadcaster; }

   // Returns the number of players who have connected to this server, but have not completed login
   int32 GetNumPlayersPendingLogin() const { return _pendingPlayerLogins.Num(); }

private:
   virtual uint8 _GetTeamIndexForNewPlayer(APlayerController* newPlayer);

   struct FPlayerStartBucket
   {
      TArray<TWeakObjectPtr<APlayerStart>> PlayerStarts;
      int32 CurrentIndex = 0;
   };

   AActor* _ChoosePlayerStartForTeam(AController* controller);
   AActor* _ChoosePlayerStartDefault(AController* controller);
   template<typename T>
   AActor* _ChoosePlayerStart(AController* controller, TConstArrayView<T> allPlayerStarts);
   void _TryInitRandomness();
   void _GenerateRandomStream();
   void _GeneratePlayerStartAreaIndexes();
   FPlayerStartBucket* _GetStartBucketForTeam(uint8 team);

private:
   UPROPERTY(EditDefaultsOnly, Category = "Classes|TAT")
   TSubclassOf<ATATToastBroadcaster> _toastBroadcasterClass;

   // are random features initialized?
   bool _isRandomnessInitialized = false;

   // random stream used by random player starts and 
   FRandomStream _randomStream;

   bool _startingAreasInitialized = false;
   TArray<int32> _availableStartingAreas;
   TMap<uint8, FPlayerStartBucket> _teamPlayerStartBuckets;

   TMap<uint8, const FLinearColor> _teamIDToColor;
   uint8 _nextTeamColorID { 0 };

   UPROPERTY(Transient)
   TObjectPtr<ATATToastBroadcaster> _toastBroadcaster;

   // Set of players that have started the login process but are not yet initialized
   TArray<FUniqueNetIdRepl, TInlineAllocator<6>> _pendingPlayerLogins;

};
