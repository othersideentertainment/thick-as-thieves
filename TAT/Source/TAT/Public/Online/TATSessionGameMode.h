// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue 
#include "CoreMinimal.h"

// tat
#include "GameFramework/TATGameMode.h"
#include "GameFramework/TATMatchPersistentTypes.h"

#include "TATSessionGameMode.generated.h"

struct FMatchPersistentRankingPlayerData;
class ATATEscapePoint;
struct FMatchPersistentData;
class ATATPlayerController;
struct FStreamableHandle;

/**
 * A base class for all in-game game modes. This will handle the setup and tear down, along with the base persistence layers.
 * Child game modes will be able to override this functionality to support their goals.
 */
UCLASS()
class TAT_API ATATSessionGameMode : public ATATGameMode
{
   GENERATED_BODY()

public:
   ATATSessionGameMode();

   virtual void PostLogin(APlayerController* newPlayer) override;
   virtual void BeginPlay() override;

	// Returns true if the player has successfully escaped, along with their zero-indexed placement in the escape order of players
	// Returns false if they have not escaped
	bool HasPlayerEscaped(const APlayerState* ps) const;
	bool GetPlayerEscapeOrderPlacement(const APlayerState* ps, int32& outEscapePlacement) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void PlayerCaught(APlayerController* caughtPlayer);

   void HandlePlayersEscaped(TConstArrayView<APlayerController*> players, ATATEscapePoint* escapePoint);
   void HandlePlayersCaught(TConstArrayView<APlayerController*> players);

   
   
	void ForceRemainingPlayersCaught();
	int GetTotalNumberOfActivePlayers() const;

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   virtual void HandlePlayerKO(APlayerController* player);
   
   virtual UClass* GetDefaultPawnClassForController_Implementation(AController* inController) override;
   virtual bool MustSpectate_Implementation(APlayerController* newPlayerController) const override;
protected:


   void _HandlePlayersEscapedOrCaught(TConstArrayView<APlayerController*> players, ATATEscapePoint* escapePoint, const bool bCaught);
   void _FinalizeMatchResults();

   virtual void _CreateAndSendClientEscapedRPC(ATATPlayerController* controller, const FMatchPersistentData& persistentData, ATATEscapePoint* escapePoint);
   void _CalculateQuestResult(FMatchPersistentData& persistentData, ATATPlayerState* tatPS, bool didEscape) const;
   void _CalculateInitialMatchPersistentData(ATATPlayerState* tatPS, FMatchPersistentData& persistentData) const;

   virtual void _UpdateOtherPlayersOfEscape(const FMatchPersistentData& persistentData,
                                            const ATATPlayerController* controller,
                                            ATATEscapePoint* escapePoint) const;

   bool _HasActivePlayers() const;

   virtual void _PerformAsyncLoads();
   
   // Tracks all players who have successfully escaped
   TArray<TWeakObjectPtr<const APlayerState>> _escapedPlayers;
   TArray<TWeakObjectPtr<const APlayerState>> _caughtPlayers;

   TSharedPtr<FStreamableHandle> _escapeOrderBonusHandle;

private:
   UFUNCTION()
   void _OnMatchFailsafeDurationReached();

   UFUNCTION()
   void _OnMatchEnd();
   
   bool _ranOnMatchEnd = false;
   FTimerHandle _matchEndTimer;
   FTimerHandle _matchEndCleanupTimer;

   float _matchStartTime;

   // Tracks the results of each player that will be put on a scoreboard. Sent to all players when the match ends
   // to display on the results screen
   UPROPERTY(Transient)
   TArray<FMatchPersistentRankingPlayerData> _rankingData;
};
