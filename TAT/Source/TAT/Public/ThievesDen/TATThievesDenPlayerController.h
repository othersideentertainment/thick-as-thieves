// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Player/TATPlayerController.h"

#include "TATThievesDenPlayerController.generated.h"

class AGameStateBase;
class UTATSavedLootInventoryUIProxyComponent;
class UTATScreenWidget;
class ATATThievesDenManager;
enum class ETATThievesDenScreen : uint8;

UCLASS()
class TAT_API ATATThievesDenPlayerController : public ATATPlayerController
{
   GENERATED_BODY()

public:
   ATATThievesDenPlayerController(const FObjectInitializer& objectInitializer);

   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void ClientSetHUD_Implementation(TSubclassOf<AHUD> newHUDClass) override;

   UFUNCTION(BlueprintCallable, Category = "Thieves Den")
   void SetDestinationMap(const TSoftObjectPtr<UWorld>& newMap);

   UFUNCTION(BlueprintCallable, Category = "Thieves Den")
   FORCEINLINE UTATSavedLootInventoryUIProxyComponent* GetSavedLootInventoryUIProxyComponent() { return SavedLootInventoryUIProxy; }

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMatchSettingsUpdated);
   /// Event fired when the server sends updated match settings to a client during match setup
   UPROPERTY(BlueprintAssignable)
   FOnMatchSettingsUpdated OnMatchSettingsUpdated;

   UFUNCTION(BlueprintCallable, Server, Reliable)
   void ServerRequestSetThievesDenScreen(ETATThievesDenScreen newScreen);

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FThievesDenScreenWidgetEvent, ETATThievesDenScreen, screenType, UTATScreenWidget*, widget);

   UPROPERTY(BlueprintAssignable)
   FThievesDenScreenWidgetEvent OnThievesDenScreenWidgetAdded;

   UPROPERTY(BlueprintAssignable)
   FThievesDenScreenWidgetEvent OnThievesDenScreenWidgetRemoved;

protected:
   UPROPERTY(EditDefaultsOnly, Category = "Lobby Screens")
   TSubclassOf<UTATScreenWidget> SetupMatchScreenWidget;

   UPROPERTY(BlueprintReadOnly)
   UTATSavedLootInventoryUIProxyComponent* SavedLootInventoryUIProxy = nullptr;

private:
   UFUNCTION()
   void _OnGameStateSet(AGameStateBase* gameState);

   UFUNCTION()
   void _OnThievesDenScreenChanged(ETATThievesDenScreen oldMatchLobbyState, ETATThievesDenScreen newMatchLobbyState);

   TSubclassOf<UTATScreenWidget> _GetScreenForState(ETATThievesDenScreen matchScreen) const;

   UFUNCTION(Server, Reliable)
   void _ServerSetDestinationMap(const TSoftObjectPtr<UWorld>& newMap);

   UFUNCTION(Client, Reliable)
   void _ClientMatchSettingsUpdated(const TArray<uint8>& serializedMatchSettings);
};
