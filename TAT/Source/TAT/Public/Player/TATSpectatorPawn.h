// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameFramework/SpectatorPawn.h"

#include "TATSpectatorPawn.generated.h"

class UTATMatchPersistenceGameInstanceSubsystem;
class UInputAction;
struct FInputActionValue;

// Spectator pawns are spawned on the client side, so we don't need to worry about replication here
UCLASS(Blueprintable, BlueprintType, notplaceable)
class TAT_API ATATSpectatorPawn : public ASpectatorPawn
{
   GENERATED_BODY()

public:
   ATATSpectatorPawn(const FObjectInitializer& objectInitializer);
   virtual void PossessedBy(AController* newController) override;
   virtual void SetupPlayerInputComponent(UInputComponent* playerInputComponent) override;
   bool _IsInEscapedState() const;

   UPROPERTY(EditDefaultsOnly, Category="TAT|Spectator|Camera Limits")
   float PitchMin {-89.9f};
   UPROPERTY(EditDefaultsOnly, Category="TAT|Spectator|Camera Limits")
   float PitchMax {89.9f};

   UPROPERTY(EditDefaultsOnly, Category="TAT|Spectator|Camera Limits")
   float YawMin {-89.9f};
   UPROPERTY(EditDefaultsOnly, Category="TAT|Spectator|Camera Limits")
   float YawMax {89.9f};

   UFUNCTION(BlueprintPure)
   APlayerState* GetFollowedPlayerState() const { return _followedPlayer.Get(); }

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCycleFollowedPlayer, APlayerState*, followedPlayerState);
   UPROPERTY(BlueprintAssignable, Category = "TAT|Spectator")
   FOnCycleFollowedPlayer OnCycleFollowedPlayer;

protected:
   void _OnCyclePrevPlayer();
   void _OnCycleNextPlayer();
   void _OnFinishEscaping();

   void _CyclePlayer(int direction);
   virtual void Tick(float deltaSeconds) override;
   APlayerState* _GetNextPlayerToSpectate(int direction) const;

private:
   UPROPERTY(Transient)
   TObjectPtr<UTATMatchPersistenceGameInstanceSubsystem> _persistenceGameInstanceSubsystem;
   
   UPROPERTY(Transient)
   TObjectPtr<APlayerState> _followedPlayer;
   
   FRotator _offset = FRotator::ZeroRotator;

   UPROPERTY(EditDefaultsOnly, Category="TAT|Spectator|Input")
   UInputAction* _cycleNextPlayerAction {};
   
   UPROPERTY(EditDefaultsOnly, Category="TAT|Spectator|Input")
   UInputAction* _cyclePrevPlayerAction {};
   
   UPROPERTY(EditDefaultsOnly, Category="TAT|Spectator|Input")
   UInputAction* _finishEscapingAction {};
};
