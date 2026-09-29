// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/MissionLobby/TATMissionLobbyScreenWidget.h"

// tat
#include "Online/TATGameState.h"
#include "Player/TATPlayerState.h"
#include "Settings/TATMatchSettingsBase.h"
#include "TATGameInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATMissionLobbyScreenWidget)
DEFINE_LOG_CATEGORY_STATIC(LogTATMissionLobbyScreenWidget, Log, All);

UTATMissionLobbyScreenWidget::UTATMissionLobbyScreenWidget()
{
   ReceiveOnGameStateFound = true;
   ReceiveOnLocalPlayerStateAdded = true;
}

void UTATMissionLobbyScreenWidget::NativeDestruct()
{
   if (ATATGameState* tatGs = GetWorld()->GetGameState<ATATGameState>())
   {
      tatGs->OnMissionOwnerPlayerStateChanged.RemoveAll(this);
   }
   GetWorld()->GetTimerManager().ClearTimer(_matchSettingsServerUpdateTimer);

   Super::NativeDestruct();
}

void UTATMissionLobbyScreenWidget::_OnGameStateFound_Implementation(ATATGameState* gameState)
{
   if (gameState->GetMissionOwnerPlayerState())
   {
      TryInitMatchSettings();
   }
   gameState->OnMissionOwnerPlayerStateChanged.AddDynamic(this, &UTATMissionLobbyScreenWidget::_OnMissionOwnerPlayerStateChanged);
}

void UTATMissionLobbyScreenWidget::_OnLocalPlayerStateAdded_Implementation(ATATPlayerState* ps)
{
   TryInitMatchSettings();
}

void UTATMissionLobbyScreenWidget::TryInitMatchSettings()
{
   ATATPlayerState* localPs = ATATPlayerState::GetLocalTATPlayerState(this);
   const ATATPlayerState* missionOwner = _TryGetMissionOwner();
   if (!missionOwner || !localPs)
   {
      return;
   }
   const bool isMissionOwner = missionOwner == localPs;
   UTATGameInstance& gameInstance = UTATGameInstance::Get(this);

   // NOTE: we only init the match settings to default values if mission owner AND not already cached (implying prior init)
   const bool shouldInitMatchSettingDefaults = MatchSettings == nullptr && isMissionOwner;

   // If mission-ownership change takes place (but we still rely on the same match-settings instance), early out
   UTATMatchSettingsBase* newMatchSettings = isMissionOwner ? localPs->GetLocalUIMatchSettings() : &gameInstance.GetMatchSettings();
   check(newMatchSettings);
   if (newMatchSettings == MatchSettings)
   {
      return;
   }
   MatchSettings = newMatchSettings;
   check(MatchSettings);

   // Unbind from OnMatchSettingsUpdated (in case we weren't mission owner, but now are)
   gameInstance.OnMatchSettingsUpdated.RemoveAll(this);

   if (shouldInitMatchSettingDefaults)
   {
      // Init default values (to handle any randomized-value settings) and update server with selections
      const FTATMatchSettingsQueryContext context = UTATMatchSettingsBase::MakeMatchSettingsQueryContextFromWorldContext(this);
      MatchSettings->InitMatchSettingsForNewMatch(context);
      localPs->ServerUpdateMatchSettings();
   }
   else
   {
      // Subscribe to replicated changes
      gameInstance.OnMatchSettingsUpdated.AddDynamic(this, &UTATMissionLobbyScreenWidget::OnMatchSettingsChangeReplicated);
   }

   OnMatchSettingsFound(shouldInitMatchSettingDefaults);
}

void UTATMissionLobbyScreenWidget::_OnMissionOwnerPlayerStateChanged(ATATPlayerState* newMissionOwner)
{
   UE_LOG(LogTATMissionLobbyScreenWidget, Verbose, TEXT("Mission owner changed to player: %s"), *GetNameSafe(newMissionOwner));
   TryInitMatchSettings();
}

void UTATMissionLobbyScreenWidget::_TrySendMatchSettingsServerUpdate()
{
   ATATPlayerState* localPs = ATATPlayerState::GetLocalTATPlayerState(this);
   if (localPs && localPs->IsMissionOwner())
   {
      UE_LOG(LogTATMissionLobbyScreenWidget, Verbose, TEXT("[%s] | Sending a match-settings update to the server..."), *localPs->GetPlayerName());
      localPs->ServerUpdateMatchSettings();
   }
}

const ATATPlayerState* UTATMissionLobbyScreenWidget::_TryGetMissionOwner() const
{
   const ATATPlayerState* localPs = ATATPlayerState::GetLocalTATPlayerState(this);
   if (!localPs)
   {
      return nullptr;
   }
   const ATATGameState* gameState = ATATGameState::GetTATGameState(this);
   if (!gameState)
   {
      return nullptr;
   }
   return gameState->GetMissionOwnerPlayerState();
}

void UTATMissionLobbyScreenWidget::OnMatchSettingsChangeReplicated_Implementation(UTATMatchSettingsBase* newMatchSettings)
{
   // Should never be called for mission owner!
#if DO_ENSURE
   const ATATPlayerState* localPs = ATATPlayerState::GetLocalTATPlayerState(this);
   ensure(localPs);
   const ATATGameState* gs = ATATGameState::GetTATGameState(this);
   ensure(gs && gs->GetMissionOwnerPlayerState() != localPs);
#endif // DO_ENSURE
}

void UTATMissionLobbyScreenWidget::ScheduleMatchSettingsServerUpdate()
{
   FTimerManager& timerManager = GetWorld()->GetTimerManager();
   if (timerManager.IsTimerActive(_matchSettingsServerUpdateTimer))
   {
      return;
   }

   // Should only be called by mission-owner player
   const ATATPlayerState* missionOwner = _TryGetMissionOwner();
   if (!missionOwner)
   {
      UE_LOG(LogTATMissionLobbyScreenWidget, Error, TEXT("ScheduleMatchSettingsServerUpdate() called before mission-owner player state found!"));
      return;
   }
   const ATATPlayerState* localPs = GetOwningPlayer()->GetPlayerState<ATATPlayerState>();
   if (localPs != missionOwner)
   {
      UE_LOG(LogTATMissionLobbyScreenWidget, Error, TEXT("ScheduleMatchSettingsServerUpdate() called for non mission-owner player (%s)!"), *GetNameSafe(localPs));
      return;
   }

   UE_LOG(LogTATMissionLobbyScreenWidget, Verbose, TEXT("Scheduling match-settings update to send in %f seconds..."), _matchSettingsServerUpdateDelaySeconds);
   timerManager.SetTimer(_matchSettingsServerUpdateTimer, this, &UTATMissionLobbyScreenWidget::_TrySendMatchSettingsServerUpdate, _matchSettingsServerUpdateDelaySeconds, false);
}
