// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Player/OSEPlayerStats.h"

// ue4
#include "GameFramework/GameStateBase.h"

#include "OSEGameState.generated.h"

class AOSEPlayerState;
class UOSEVoiceOverControllerComponent;

UCLASS()
class OSECORE_API AOSEGameState : public AGameStateBase
{
   GENERATED_BODY()

public:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerArrayChanged, AOSEPlayerState*, playerState);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSessionStatsChanged);

public:
   AOSEGameState(const FObjectInitializer& objectInitializer);

   // static
   UFUNCTION(BlueprintPure, Category = "GameState|OSE", meta = (WorldContext = "contextObj"))
   static AOSEGameState* GetOSEGameState(const UObject* contextObj);

   UPROPERTY(BlueprintAssignable, Category = "GameState|OSE")
   FOnPlayerArrayChanged OnPlayerStateAdded;
   UPROPERTY(BlueprintAssignable, Category = "GameState|OSE")
   FOnPlayerArrayChanged OnLocalPlayerStateAdded;
   UPROPERTY(BlueprintAssignable, Category = "GameState|OSE")
   FOnPlayerArrayChanged OnPlayerStateRemoved;

   UFUNCTION(BlueprintPure, Category = "GameState|OSE")
   const TArray<AOSEPlayerState*>& GetOSEPlayerStates() const { return _osePlayerStates; }

   UFUNCTION(BlueprintGetter, Category = "GameState|OSE")
   UOSEVoiceOverControllerComponent* GetVOController() const { return _voController; }

   // get/set session stats
   UFUNCTION(BlueprintPure, Category = "GameState|OSE|Session Stats")
   const FOSEPlayerStats& GetSessionStats() const { return _sessionStats; }
   void AuthorityUpdateSessionStatInt(const FGameplayTag& statTag, int updateValue);
   UPROPERTY(BlueprintAssignable, Category = "GameState|OSE|Session Stats")
   FOnSessionStatsChanged OnSessionStatsChanged;

protected:
   // Overrides
   virtual void HandleBeginPlay() override;
   virtual void AddPlayerState(APlayerState* playerState) override;
   virtual void RemovePlayerState(APlayerState* playerState) override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   // for our subclasses
   virtual void _OnOSEPlayerStateAdded(AOSEPlayerState* playerState) { }
   virtual void _OnOSEPlayerStateRemoved(AOSEPlayerState* playerState) { }

private:
   void _SortOSEPlayerStates();

   UFUNCTION()
   void _OnRep_SessionStats();
   void _BroadcastSessionStatsChanged();

private:
   // a duplicate of PlayerArray pre-cast and stable sorted on server + clients
   UPROPERTY(Transient)
   TArray<AOSEPlayerState*> _osePlayerStates;

   UPROPERTY(Transient, BlueprintGetter = GetVOController, meta = (AllowPrivateAccess = "true"))
   UOSEVoiceOverControllerComponent* _voController;
   
   // session stats
   UPROPERTY(ReplicatedUsing = _OnRep_SessionStats)
   FOSEPlayerStats _sessionStats;
};
