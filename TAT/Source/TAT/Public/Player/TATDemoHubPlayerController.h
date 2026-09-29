// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Player/TATPlayerController.h"
#include "GameFramework/TATMatchLobbyTypes.h"

#include "TATDemoHubPlayerController.generated.h"

class AGameStateBase;
class UTATScreenWidget;

UCLASS()
class TAT_API ATATDemoHubPlayerController : public ATATPlayerController
{
	GENERATED_BODY()

public:

   // the demo hub does not wait for a character, since there is none
   virtual bool IsCharacterReady() const override { return true; }
   virtual void ClientSetHUD_Implementation(TSubclassOf<AHUD> newHUDClass) override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

   UFUNCTION(BlueprintCallable)
   void SetDestinationMap(const TSoftObjectPtr<UWorld>& newMap, FSoftClassPath mapPrefix);

private:
   UFUNCTION()
   void _OnMatchLobbyStateChanged(ETATMatchLobbyState oldMatchLobbyState, ETATMatchLobbyState newMatchLobbyState);

   UFUNCTION()
   void _OnGameStateSet(AGameStateBase* gameState);

   TSubclassOf<UTATScreenWidget> _GetScreenForState(ETATMatchLobbyState matchLobbyState) const;

   UFUNCTION(Server, Reliable)
   void _ServerSetDestinationMap(const TSoftObjectPtr<UWorld>& newMap, const FSoftClassPath& mapPrefix);

protected:
   UPROPERTY(EditDefaultsOnly, Category = "Lobby Screens")
   TSubclassOf<UTATScreenWidget> SetupMatchScreen;

   UPROPERTY(EditDefaultsOnly, Category = "Lobby Screens")
   TSubclassOf<UTATScreenWidget> CharacterSelectScreen;
};
