// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "UI/TATScreenWidget.h"

#include "TATMissionLobbyScreenWidget.generated.h"

class ATATGameState;
class ATATPlayerState;
class UTATMatchSettingsBase;

/// Screen used in the mission lobby to visualize match settings. 
/// Responsible for:
/// * Listening for mission-owner change
/// * Retrieving the appropriate TATMatchSettings instance and piping up to BP to initialize widgets
/// * Sending match-setting updates to the server (with a delay-buffer configurable via _matchSettingsServerUpdateDelaySeconds)
UCLASS()
class TAT_API UTATMissionLobbyScreenWidget : public UTATScreenWidget
{
   GENERATED_BODY()

   UTATMissionLobbyScreenWidget();

   // From UUserWidget
   virtual void NativeDestruct() override;

   // From UTATUserWidget
   void _OnGameStateFound_Implementation(ATATGameState* gameState);
   void _OnLocalPlayerStateAdded_Implementation(ATATPlayerState* ps);

protected:
   void TryInitMatchSettings();

   // Called for other players when a match-settings change made by the mission owner replicates.
   UFUNCTION(BlueprintNativeEvent, Category = "Match Settings")
   void OnMatchSettingsChangeReplicated(UTATMatchSettingsBase* newMatchSettings);
   void OnMatchSettingsChangeReplicated_Implementation(UTATMatchSettingsBase* newMatchSettings);

   // Called when MatchSettings has been initialized, or when mission-owner change requires us to point to different instance
   // If wasJustInitializedToDefaults = true, MatchSettings has just been initialized with default values (implies local player = mission owner).
   // If false, it now points to a previously-initialized instance (selected w.r.t mission ownership)
   UFUNCTION(BlueprintImplementableEvent, Category = "Match Settings")
   void OnMatchSettingsFound(bool wasJustInitializedToDefaults);

   // Waits for _matchSettingsServerUpdateDelaySeconds, then sends a match-settings update to the server. Intended to prevent user interaction from spamming RPCs.
   // Multiple calls within the delay period are batched together as a shared update.
   // NOTE: should only be called by mission-owner player!
   UFUNCTION(BlueprintCallable, Category = "Match Settings")
   void ScheduleMatchSettingsServerUpdate();

protected:
   UPROPERTY(Transient, BlueprintReadOnly)
   UTATMatchSettingsBase* MatchSettings = nullptr;

private:
   UFUNCTION()
   void _OnMissionOwnerPlayerStateChanged(ATATPlayerState* newMissionOwner);

   UFUNCTION()
   void _TrySendMatchSettingsServerUpdate();

   const ATATPlayerState* _TryGetMissionOwner() const;

private:
   UPROPERTY(EditDefaultsOnly, Category = "Match Settings", meta = (UIMin = 0, UIMax = 1, ClampMin = 0, ClampMax = 1, Units = "Seconds"))
   float _matchSettingsServerUpdateDelaySeconds = .25f;

   FTimerHandle _matchSettingsServerUpdateTimer;
};
