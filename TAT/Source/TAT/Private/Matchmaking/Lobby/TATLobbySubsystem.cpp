// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Matchmaking/Lobby/TATLobbySubsystem.h"

// tat
#include "GameFramework/TATWorldSettings.h"
#include "Quests/TATContractPrioritizer.h"
#include "Quests/TATContractSelectionComponent.h"
#include "Settings/TATMatchSettings.h"
#include "Settings/TATMatchSettingsBase.h"
#include "Variation/DemoHubMissionMgr.h"
#include "TATGameInstance.h"

// ose
#include "Online/OSEGameState.h"
#include "Player/OSEPlayerState.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLobbySubsystem)

class ATATWorldSettings;

bool UTATLobbySubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   if (!Super::ShouldCreateSubsystem(outer))
   {
      return false;
   }

   UWorld* world = CastChecked<UWorld>(outer);

   const ATATWorldSettings* worldSettings = CastChecked<ATATWorldSettings>(world->GetWorldSettings());
   return worldSettings->MapType == ETATMapType::Hub;
}

void UTATLobbySubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);
}

void UTATLobbySubsystem::Deinitialize()
{
   Super::Deinitialize();

   if (_source)
   {
      _source->Cleanup();
   }
}

void UTATLobbySubsystem::OnWorldBeginPlay(UWorld& inWorld)
{
   Super::OnWorldBeginPlay(inWorld);

   auto getSourceClass = [this]() -> UClass*
      {
         // NOTE: formerly depended if pragma
         return UTATReplicatedLobbySource::StaticClass();
      };

   UClass* lobbyClass = getSourceClass();
   _source = NewObject<UTATLobbySource>(this, lobbyClass);
   _source->OnContractChanged.AddWeakLambda(this, [this]() {OnContractChanged.Broadcast(); });
   _source->OnMissionChanged.AddWeakLambda(this, [this]() { OnMissionChanged.Broadcast(); });
   _source->OnMatchModeChanged.AddWeakLambda(this, [this]() {OnMatchModeChanged.Broadcast(); });
   _source->OnDifficultyChanged.AddWeakLambda(this, [this]() {OnDifficultyChanged.Broadcast(); });
   _source->OnWorldBeginPlay();
}

const FGameplayTag& UTATLobbySubsystem::GetContract() const
{
   return _source ? _source->GetContract() : FGameplayTag::EmptyTag;
}

const FGameplayTag& UTATLobbySubsystem::GetMission() const
{
   return _source ? _source->GetMission() : FGameplayTag::EmptyTag;
}

bool UTATLobbySubsystem::GetMatchMode(ETATMatchMode& found) const
{
   if (_source && _source->GetMatchMode().IsSet())
   {
      found = _source->GetMatchMode().GetValue();
      return true;
   }
   return false;
}

bool UTATLobbySubsystem::GetDifficulty(ETATDifficulty& found) const
{
   if (_source && _source->GetDifficulty().IsSet())
   {
      found = _source->GetDifficulty().GetValue();
      return true;
   }
   return false;
}

bool UTATLobbySubsystem::DoesSupportWorldType(const EWorldType::Type worldType) const
{
   return worldType == EWorldType::Game || worldType == EWorldType::PIE;
}

void UTATLobbySource::OnWorldBeginPlay()
{
   // NOTE: We could get the map more directly via sources, but I don't currently want to create competing sources of truth,
   //       so just proxying the mission manager for now
   if (ATATDemoHubMissionMgr* missionMgr = ATATDemoHubMissionMgr::GetDemoHubMissionManager(this))
   {
      missionMgr->OnSelectedMapChanged.AddUniqueDynamic(this, &ThisClass::_RefreshMap);
   }
}

void UTATLobbySource::Cleanup()
{
}

void UTATLobbySource::_SetMatchMode(ETATMatchMode matchMode)
{
   if (_matchMode != matchMode)
   {
      _matchMode = matchMode;
      OnMatchModeChanged.Broadcast();
   }
}

void UTATLobbySource::_SetDifficulty(ETATDifficulty difficulty)
{
   if (difficulty != _difficulty)
   {
      _difficulty = difficulty;
      OnDifficultyChanged.Broadcast();
   }
}

void UTATLobbySource::_SetContract(const FGameplayTag& newContract)
{
   if (_contract != newContract)
   {
      _contract = newContract;
      OnContractChanged.Broadcast();
   }
}

void UTATLobbySource::_SetMission(const FGameplayTag& newMission)
{
   if(_mission != newMission)
   {
      _mission = newMission;
      OnMissionChanged.Broadcast();
   }
}

void UTATLobbySource::_SetMap(const TSoftObjectPtr<UWorld>& newMap)
{
   if (_map != newMap)
   {
      _map = newMap;
      OnMapChanged.Broadcast();
      _OnMapChanged();
   }
}

void UTATLobbySource::_RefreshMap()
{
   // NOTE: We could get the map more directly via sources, but I don't currently want to create competing sources of truth,
   //       so just proxying the mission manager for now
   if (ATATDemoHubMissionMgr* missionMgr = ATATDemoHubMissionMgr::GetDemoHubMissionManager(this))
   {
      _SetMap(missionMgr->GetSelectedMap());
   }
}

void UTATReplicatedLobbySource::OnWorldBeginPlay()
{
   Super::OnWorldBeginPlay();

   // This heuristic would not be correct if we added new game modes, but we know that we won't (unfortunately)
   _SetMatchMode(GetWorld()->IsNetMode(NM_Standalone) ? ETATMatchMode::Offline : ETATMatchMode::Coop);

   if (AGameStateBase* gameState = GetWorld()->GetGameState())
   {
      _OnGameStateSet(gameState);
   }
   else
   {
      GetWorld()->GameStateSetEvent.AddUObject(this, &ThisClass::_OnGameStateSet);
   }
}

void UTATReplicatedLobbySource::Cleanup()
{
   Super::Cleanup();
}

void UTATReplicatedLobbySource::_OnGameStateSet(AGameStateBase* gameStateBase)
{
   if (AOSEGameState* gameState = Cast<AOSEGameState>(gameStateBase))
   {
      gameState->OnPlayerStateAdded.AddUniqueDynamic(this, &ThisClass::_OnPlayerStateAdded);
      gameState->OnPlayerStateRemoved.AddUniqueDynamic(this, &ThisClass::_OnPlayerStateRemoved);
      for (AOSEPlayerState* playerState : gameState->GetOSEPlayerStates())
      {
         _BindToPlayer(playerState);
      }

      _RefreshContracts();
   }
}

void UTATReplicatedLobbySource::_OnPlayerStateAdded(AOSEPlayerState* playerState)
{
   _BindToPlayer(playerState);
   _RefreshContracts();
}

void UTATReplicatedLobbySource::_OnPlayerStateRemoved(AOSEPlayerState* playerState)
{
   _RefreshContracts();
}

void UTATReplicatedLobbySource::_OnPlayerContractChanged(FGameplayTag contractTag)
{
   _RefreshContracts();
}

void UTATReplicatedLobbySource::_RefreshContracts()
{
   if (const AOSEGameState* gameState = GetWorld()->GetGameState<AOSEGameState>())
   {
      FTATContractPrioritizer prioritizer(this, GetMap());
      for (const AOSEPlayerState* playerState : gameState->GetOSEPlayerStates())
      {
         if (const UTATContractSelectionComponent* selectionComponent = playerState->GetComponentByClass<UTATContractSelectionComponent>())
         {
            prioritizer.AddContract(selectionComponent->GetSelectedContract());
         }
      }
      _SetContract(prioritizer.GetChosenContract());
   }
}

void UTATReplicatedLobbySource::_OnMapChanged()
{
   Super::_OnMapChanged();
   _RefreshContracts();
}

void UTATReplicatedLobbySource::_RefreshMap()
{
   Super::_RefreshMap();

   if (ATATDemoHubMissionMgr* missionMgr = ATATDemoHubMissionMgr::GetDemoHubMissionManager(this))
   {
      _SetMission(missionMgr->GetMissionTag());

      TOptional<ETATDifficulty> difficulty = missionMgr->GetDifficulty();
      if (difficulty.IsSet())
      {
         _SetDifficulty(difficulty.GetValue());
      }
   }
}

void UTATReplicatedLobbySource::_BindToPlayer(const AOSEPlayerState* playerState)
{
   if (UTATContractSelectionComponent* selectionComponent = playerState->GetComponentByClass<UTATContractSelectionComponent>())
   {
      selectionComponent->OnSelectedContractChanged.AddUniqueDynamic(this, &ThisClass::UTATReplicatedLobbySource::_OnPlayerContractChanged);
   }
}
