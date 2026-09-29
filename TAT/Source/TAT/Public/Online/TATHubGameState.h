// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "GameFramework/TATMatchLobbyTypes.h"
#include "Online/TATGameState.h"
#include "Developer/TATEditorSettings.h"

#include "TATHubGameState.generated.h"


UCLASS()
class TAT_API ATATHubGameState : public ATATGameState
{
   GENERATED_BODY()

public:
   // static
   UFUNCTION(BlueprintPure, Category = "Game State|TAT|Hub", meta = (WorldContext = "contextObj"))
   static ATATHubGameState* GetTATHubGameState(const UObject* contextObj);
   static ATATHubGameState* Get(const UObject& contextObj);

   ATATHubGameState(const FObjectInitializer& objectInitializer);

   // from AActor
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   void AuthoritySetMatchLobbyState(ETATMatchLobbyState matchLobbyState);
   ETATMatchLobbyState GetMatchLobbyState() const { return _matchLobbyState; }

   UPROPERTY(BlueprintAssignable, Category = "Game State|TAT|Hub")
   FOnPlayerArrayChanged OnPlayerStateChanged;
private:
   UFUNCTION()
   void _OnRep_MatchLobbyState(ETATMatchLobbyState oldState);
   
public:
   DECLARE_MULTICAST_DELEGATE_TwoParams(FOnMatchLobbyStateChanged, ETATMatchLobbyState, ETATMatchLobbyState);
   FOnMatchLobbyStateChanged OnMatchLobbyStateChanged;

private:
   // State indicating which screen should be visible
   UPROPERTY(Transient, ReplicatedUsing=_OnRep_MatchLobbyState)
   ETATMatchLobbyState _matchLobbyState = ETATMatchLobbyState::None;
};
