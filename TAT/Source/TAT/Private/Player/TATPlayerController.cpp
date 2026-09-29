// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Player/TATPlayerController.h"

// tat
#include "TATGameInstance.h"
#include "AI/TATAIStateWorldSubsystem.h"
#include "Damage/TATLocalDamageTrackerComponent.h"
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATCheatManager.h"
#include "GameFramework/TATMatchPersistenceGameInstanceSubsystem.h"
#include "GameFramework/TATMatchResultsSaveContext.h"
#include "GameFramework/TATTravelMgr.h"
#include "GameFramework/TATWorldSettings.h"
#include "Graphics/TATPlayerPostProcStackComponent.h"
#include "Input/TATEnhancedInputComponent.h"
#include "Online/TATGameState.h"
#include "Player/TATLocalPlayerStateWorldSubsystem.h"
#include "Player/TATPlayerCameraManager.h"
#include "Player/TATPlayerState.h"
#include "Player/AimAssist/TATAimAssistComponent.h"
#include "Player/Perception/TATPlayerPerceptionManagerComponent.h"
#include "SaveGame/TATSaveGame.h"
#include "Tools/TATReticleStateComponent.h"
#include "UI/TATHUD.h"
#include "UI/TATScreenMgr.h"
#include "Indicators/TATThiefVisionSubsystem.h"
#include "GameFramework/RespawnAreas/TATRespawnAreaOverlapVolume.h"
#include "GameFramework/RespawnAreas/TATRespawnAreaSpawnLocation.h"
#include "PingSystem/TATPingSystemComponent.h"
#include "Analytics/TATLocalPlayerAnalyticsSubsystem.h"

// ose
#include "OSECommon.h"
#include "Character/OSECharacterBase.h"
#include "Abilities/OSEAbilitySystemComponent.h"

// ue4
#include "AbilitySystemGlobals.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerInput.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Net/UnrealNetwork.h"
#include "Online/OnlineSessionNames.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPlayerController)

DEFINE_LOG_CATEGORY_STATIC(LogTATPlayerController, Log, All);

ATATPlayerController::ATATPlayerController(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer
      .SetDefaultSubobjectClass<UTATAimAssistComponent>(TEXT("AimAssist"))
      .SetDefaultSubobjectClass<UTATPingSystemComponent>(TEXT("PingSystem")))
{
   PlayerCameraManagerClass = ATATPlayerCameraManager::StaticClass();
   CheatClass = UTATCheatManager::StaticClass();

   ReticleState = CreateDefaultSubobject<UTATReticleStateComponent>(TEXT("ReticleState"));
   PlayerPostProcStack = CreateDefaultSubobject<UTATPlayerPostProcStackComponent>(TEXT("PlayerPostProcStack"));
   PlayerPerceptionComponent = CreateDefaultSubobject<UTATPlayerPerceptionManagerComponent>(TEXT("PlayerPerception"));

   _DamageTracker = CreateDefaultSubobject<UTATLocalDamageTrackerComponent>(TEXT("DamageTracker"));
}

void ATATPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;

   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _thiefVisionIndicators, params);

   DOREPLIFETIME(ATATPlayerController, _respawnPointActor);
   DOREPLIFETIME_CONDITION(ATATPlayerController, _currentRespawnAreaOverlapVolumes, COND_OwnerOnly);
   DOREPLIFETIME_CONDITION(ATATPlayerController, _aiDetectedInCurrentBestRespawnArea, COND_OwnerOnly);
}

/* static */
ATATPlayerController* ATATPlayerController::GetTATPlayerController(const UObject* contextObj, int index)
{
   return UOSECommon::GetPlayerControllerAtIndex<ATATPlayerController>(contextObj, index);
}

/* static */
ATATPlayerController* ATATPlayerController::GetLocalTATPlayerController(const UObject* contextObj)
{
   return UOSECommon::GetLocalPlayerController<ATATPlayerController>(contextObj);
}

ATATHUD* ATATPlayerController::GetTATHUD() const
{
   return Cast<ATATHUD>(GetHUD());
}

UTATScreenMgr* ATATPlayerController::GetTATScreenMgr() const
{
   return _screenMgr;
}

ATATPlayerState* ATATPlayerController::GetTATPlayerState() const
{
   return GetPlayerState<ATATPlayerState>();
}

void ATATPlayerController::ClientAboutToChangeMaps_Implementation()
{
   // remove all existing screens from our screen stack
   if (_screenMgr)
   {
      _screenMgr->RemoveAllScreensFromStack();
   }

   // tell travel mgr to throw up our loading screen, this will be removed automatically as soon as we leave the current map
   UTATGameInstance& gameInstance = UTATGameInstance::Get(this);
   UTATTravelMgr& travelMgr = gameInstance.GetTravelMgr();
   travelMgr.ShowLoadingScreen();
}

void ATATPlayerController::ClientStashMatchData_Implementation(const FMatchPersistentData& data)
{
   UTATMatchPersistenceGameInstanceSubsystem* matchPersistenceGameInstanceSubsystem = GetGameInstance()->GetSubsystem<UTATMatchPersistenceGameInstanceSubsystem>();
   if(matchPersistenceGameInstanceSubsystem)
   {
      matchPersistenceGameInstanceSubsystem->SetMatchPersistentData(data);
   }

   // Apply to save game
   UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(this);
   if (ensure(saveGame))
   {
      if (UTATLocalPlayerAnalyticsSubsystem* analyticsSubsystem = GetLocalPlayer()->GetSubsystem<UTATLocalPlayerAnalyticsSubsystem>())
      {
         analyticsSubsystem->HandleMatchEnd(data);
      }
      TATMatchResultsSaveContext::Apply(data, saveGame);

      if (matchPersistenceGameInstanceSubsystem)
      {
         matchPersistenceGameInstanceSubsystem->SetXPGainedData(TATMatchResultsSaveContext::ApplyXP(data, saveGame));
      }
   }
   
   BP_OnReceivedMatchPersistentData();

   ClientGotoState(NAME_Spectating);
}

void ATATPlayerController::ClientNotifyAnotherPlayerEscaped_Implementation(
   const FString& playerName,
   ATATEscapePoint* escapeRoute,
   int lootValue)
{
   OnAnotherPlayerEscaped(playerName, escapeRoute, lootValue);
}

void ATATPlayerController::ClientNotifyLeaderboardData_Implementation(const TArray<FMatchPersistentRankingPlayerData>& rankingData)
{
   if (UTATMatchPersistenceGameInstanceSubsystem* matchPersistenceGameInstanceSubsystem = GetGameInstance()->GetSubsystem<UTATMatchPersistenceGameInstanceSubsystem>())
   {
      matchPersistenceGameInstanceSubsystem->SetPersistentPlayerRankingData(rankingData);
   }
}

void ATATPlayerController::ClientFinishEscaping_Implementation()
{
   static const FText returnReason = FText::FromString("Escaped");
   if (UTATProjectSettings::Get().EnableThievesDen)
   {
      ClientReturnToThievesDen(returnReason);
   }
   else
   {
      ClientReturnToMainMenuWithTextReason(returnReason);
   }
}

void ATATPlayerController::ClientReturnToThievesDen_Implementation(const FText& returnReason)
{
   if (UTATGameInstance* gameInstance = GetGameInstance<UTATGameInstance>())
   {
      gameInstance->ReturnToThievesDen(returnReason);
   }
   else
   {
      UWorld* world = GetWorld();
      check(world != nullptr);
      GEngine->HandleDisconnect(world, world->GetNetDriver());
   }
}

void ATATPlayerController::OnNetCleanup(UNetConnection* connection)
{
   auto isEarlyMatchDisconnect = [this]()
   {
      const ATATPlayerState* ps = GetTATPlayerState();
      if (ps == nullptr)
      {
         return false;
      }

      if (ps->HasEscapedOrBeenCaptured())
      {
         return false;
      }

      const UWorld* world = GetWorld();
      if (world->bIsTearingDown)
      {
         return false;
      }

      if (const ATATWorldSettings* worldSettings = ATATWorldSettings::GetTATWorldSettings(this);
         worldSettings == nullptr || (worldSettings->MapType != ETATMapType::Mission && worldSettings->MapType != ETATMapType::Developer))
      {
         return false;
      }

      return true;
   };

   if (isEarlyMatchDisconnect())
   {
      ATATPlayerState* ps = GetTATPlayerState();
      check(ps);
      ps->AuthorityOnEarlyDisconnect();
   }
   
   
   Super::OnNetCleanup(connection);
}

void ATATPlayerController::SetPawn(APawn* inPawn)
{
   APawn* oldPawn = GetPawn();

   Super::SetPawn(inPawn);

   if (inPawn && _approximateInitialSpawnLocation.IsZero())
   {
      _approximateInitialSpawnLocation = inPawn->GetActorLocation();
   }
}

bool ATATPlayerController::IsCharacterReady() const
{
   const AOSECharacterBase* character = GetPawn<AOSECharacterBase>();
   return character && character->IsCharacterReady();
}

void ATATPlayerController::SetLookSpeedMultiplier(FName multiplierId, float mouseMultiplier, float gamepadMultiplier)
{
   if (UTATEnhancedInputComponent* enhancedInputComponent = _GetEnhancedInputComponent())
   {
      enhancedInputComponent->SetLookSpeedMultiplier(multiplierId, mouseMultiplier, gamepadMultiplier);
   }
}

void ATATPlayerController::ClearLookSpeedMultiplier(FName multiplierId)
{
   if (UTATEnhancedInputComponent* enhancedInputComponent = _GetEnhancedInputComponent())
   {
      enhancedInputComponent->ClearLookSpeedMultiplier(multiplierId);
   }
}

void ATATPlayerController::HandleReactToOwnStim(const FGameplayTag& stimTag, float loudness)
{
   if(ITATHearingStimSourceReactor* reactor = Cast<ITATHearingStimSourceReactor>(GetPawn()))
   {
      reactor->HandleReactToOwnStim(stimTag, loudness);
   }
}

void ATATPlayerController::ClientJoinOnlineSession_Implementation(const FString& sessionId)
{
   IOnlineSessionPtr sessionInterface = Online::GetSessionInterface(GetWorld());
   if (!sessionInterface.IsValid())
   {
      UE_LOG(LogTATPlayerController, Error, TEXT("ClientJoinOnlineSession: Failed to get session interface"));
      return;
   }

   const TSharedRef<FOnlineSessionSearch> searchSettings = MakeShared<FOnlineSessionSearch>();
   searchSettings->MaxSearchResults = 1;

   // SteamOSS doesn't handle custom search parameters (see FOnlineAsyncTaskSteamFindServerBase::CreateQuery)
   // nor querying for sessions by session identifier, so we use the "map name" parameter to uniquely identify
   // the server session; see also UTATDedicatedServerManagerOnlineSession::ServerReadyToAcceptPlayers
   searchSettings->QuerySettings.Set(SETTING_MAPNAME, sessionId, EOnlineComparisonOp::Equals);

   const TSharedRef<FDelegateHandle> findHandle = MakeShared<FDelegateHandle>();
   *findHandle = sessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
      FOnFindSessionsCompleteDelegate::CreateLambda([sessionInterface, findHandle, searchSettings](bool bSuccess)
      {
         if (!bSuccess || searchSettings->SearchResults.IsEmpty())
         {
            UE_LOG(LogTATPlayerController, Error, TEXT("ClientJoinOnlineSession: Failed to find server session"));
         }
         else
         {
            const FOnlineSessionSearchResult& serverSession = searchSettings->SearchResults[0];

            const TSharedRef<FDelegateHandle> joinHandle = MakeShared<FDelegateHandle>();
            *joinHandle = sessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
               FOnJoinSessionCompleteDelegate::CreateLambda([sessionInterface, joinHandle](FName, EOnJoinSessionCompleteResult::Type result)
               {
                  if (result != EOnJoinSessionCompleteResult::Success)
                  {
                     UE_LOG(LogTATPlayerController, Error, TEXT("ClientJoinOnlineSession: Failed to join server session (%s)"), LexToString(result));
                  }

                  sessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(*joinHandle);
               }));

            if (!sessionInterface->JoinSession(0, NAME_GameSession, serverSession))
            {
               UE_LOG(LogTATPlayerController, Error, TEXT("ClientJoinOnlineSession: Failed to join server session"));
               sessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(*joinHandle);
            }
         }

         sessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(*findHandle);
      }));

   if (!sessionInterface->FindSessions(0, searchSettings))
   {
      UE_LOG(LogTATPlayerController, Error, TEXT("ClientJoinOnlineSession: Failed to find server session"));
      sessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(*findHandle);
   }
}

void ATATPlayerController::HandleLocalPlayerGeneratedNoiseStim(const FTATAINoiseEvent& stim)
{
   check(IsLocalController());
   OnLocalPlayerGeneratedNoiseStim.Broadcast(stim);
}

void ATATPlayerController::_HandleInstantDamageTaken(FTATDamageWithType damage, FTATLocalDamageSource source)
{
   const APawn* currentPawn = GetPawn();
   if (currentPawn == nullptr) return;
   if (UTATAnalyticsManager* analyticsManager = GetGameInstance()->GetSubsystem<UTATAnalyticsManager>())
   {
      FTATAnalyticsCustomFields fields;
      fields.Set(TEXT("Location"), currentPawn->GetActorLocation().ToCompactString());
      fields.Set(TEXT("SourceLocation"), source.Origin.ToCompactString());
      fields.Set(TEXT("InstigatorName"), GetNameSafe(source.Instigator));
      fields.Set(TEXT("Amount"), damage.DamageAmount);
      fields.Set(TEXT("Type"), damage.DamageType);
      analyticsManager->OnDesignEventWithCustomFields(TEXT("PlayerDamage"), fields);
   }
}

void ATATPlayerController::BeginPlay()
{
   Super::BeginPlay();

   // start in the camera type defined by world settings
   const ATATWorldSettings& worldSettings = ATATWorldSettings::Get(this);
   EPlayerCameraMode cameraMode = worldSettings.MapStartCameraMode;
   SetPlayerCameraMode(cameraMode);
   if (GetPlayerCameraMode() == EPlayerCameraMode::ThirdPerson)
   {
      FTimerHandle timerHandle;
      GetWorld()->GetTimerManager().SetTimer(timerHandle, this, &ATATPlayerController::_OnEnterFirstPersonCameraTimeout, SecondsToEnterFirstPersonCamera);
   }

   if (IsLocalController())
   {
      // Connect thief vision indicators with their change callbacks for this controller
      _thiefVisionIndicators.OwningLocalPlayerControllerWeak = this;
      _SpawnRespawnMarker();
      _DamageTracker->OnInstantDamageTaken.AddUniqueDynamic(this, &ThisClass::_HandleInstantDamageTaken);
   }

#if OSE_CHEATS_ENABLED
   EnableCheats();
#endif
}

void ATATPlayerController::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   Super::EndPlay(endPlayReason);
}

void ATATPlayerController::ClientSetHUD_Implementation(TSubclassOf<AHUD> newHUDClass)
{
   Super::ClientSetHUD_Implementation(newHUDClass);

   // spawn our screen mgr right after the hud is spawned, seems logically connected even though they're not identical
   _TrySpawnScreenMgr();
}

void ATATPlayerController::TickActor(float deltaTime, enum ELevelTick tickType, FActorTickFunction& thisTickFunction)
{
   Super::TickActor(deltaTime, tickType, thisTickFunction);

   const ACharacter* controlledCharacter = Cast<ACharacter>(GetPawn());
   if (controlledCharacter)
   {
      const UCharacterMovementComponent* characterMovementComponent = controlledCharacter->GetCharacterMovement();
      check(IsValid(characterMovementComponent));

      _TickPlayerMovementState(characterMovementComponent);
   }

   _TickLocalSubsystemState();

   if (UTATThiefVisionSubsystem* thiefVisionSubsystem = GetWorld()->GetSubsystem<UTATThiefVisionSubsystem>())
   {
      bool newThiefVisionEnabled = false;

      if (const IGameplayTagAssetInterface* characterTags = Cast<IGameplayTagAssetInterface>(controlledCharacter))
      {
         newThiefVisionEnabled = characterTags->HasMatchingGameplayTag(UTATProjectSettings::Get().ThiefVisionStatusTag);
      }

      // Notify the subsystem if thief vision was toggled on or off
      if (newThiefVisionEnabled != _thiefVisionEnabled)
      {
         _thiefVisionEnabled = newThiefVisionEnabled;
         thiefVisionSubsystem->NotifyThiefVisionStatusChanged(this, newThiefVisionEnabled);
      }

      if (HasAuthority())
      {
         // Poll the thief vision subsystem to find out which indicators should be visible to the player this frame
         if (thiefVisionSubsystem->AuthorityUpdateThiefVisionIndicatorsForPlayer(this, _thiefVisionIndicators))
         {
            MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _thiefVisionIndicators, this);
            _OnThiefVisionIndicatorsUpdated();
         }
      }

      if (IsLocalController())
      {
         // Refresh indicator visibility - make indicators visible or not based on distance and play the relevant transition effects to hide/show them
         thiefVisionSubsystem->ClientSyncThiefVisionIndicatorVisibility(this);
      }
   }
}

void ATATPlayerController::SetPlayer(UPlayer* inPlayer)
{
   Super::SetPlayer(inPlayer);

   if (ATATPlayerState* playerState = GetPlayerState<ATATPlayerState>();
      playerState && NetConnection)
   {
      playerState->OnNetConnectionSet();
   }
}

void ATATPlayerController::SetupInputComponent()
{
   Super::SetupInputComponent();

#if (OSE_CHEATS_ENABLED)
   {
      static bool sBindingsAdded = false;
      if (!sBindingsAdded)
      {
         sBindingsAdded = true;

         UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping("TATCheat_ChangeCharacter_1", EKeys::H, true, false, true));
         UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping("TATCheat_ChangeCharacter_2", EKeys::J, true, false, true));
         UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping("TATCheat_ChangeCharacter_3", EKeys::K, true, false, true));

         UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping("TATCheat_CameraMode_Toggle1P", EKeys::Up, true, false, true));
         UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping("TATCheat_CameraMode_Toggle3P", EKeys::Down, true, false, true));
      }

      InputComponent->BindAction("TATCheat_ChangeCharacter_1", IE_Pressed, this, &ATATPlayerController::_TATCheat_ChangeCharacter_1);
      InputComponent->BindAction("TATCheat_ChangeCharacter_2", IE_Pressed, this, &ATATPlayerController::_TATCheat_ChangeCharacter_2);
      InputComponent->BindAction("TATCheat_ChangeCharacter_3", IE_Pressed, this, &ATATPlayerController::_TATCheat_ChangeCharacter_3);

      InputComponent->BindAction("TATCheat_CameraMode_Toggle1P", IE_Released, this, &ATATPlayerController::_TATCheat_CameraMode_Toggle1P);
      InputComponent->BindAction("TATCheat_CameraMode_Toggle3P", IE_Released, this, &ATATPlayerController::_TATCheat_CameraMode_Toggle3P);
   }
#endif //OSE_CHEATS_ENABLED
}

void ATATPlayerController::GetSeamlessTravelActorList(bool toEntry, TArray<class AActor*>& actorList)
{
   Super::GetSeamlessTravelActorList(toEntry, actorList);

   // clients can add actors to seamless travel here
}

void ATATPlayerController::NotifyLoadedWorld(FName worldPackageName, bool finalDest)
{
   // note: called on client and server both

   Super::NotifyLoadedWorld(worldPackageName, finalDest);

   // disable movement input on the transition map
   const bool isOnTransitionMap = !finalDest;
   SetIgnoreMoveInput(isOnTransitionMap);

   if (isOnTransitionMap)
   {
      SetPlayerCameraMode(EPlayerCameraMode::ThirdPerson);
      OnEnteredTransitionMap.Broadcast();
   }
}

void ATATPlayerController::SeamlessTravelFrom(APlayerController* oldPC)
{
   Super::SeamlessTravelFrom(oldPC);

   // note: only called server-side, from GameMode
}

void ATATPlayerController::SeamlessTravelTo(APlayerController* newPC)
{
   Super::SeamlessTravelTo(newPC);

   // note: only called server-side, from GameMode
}

bool ATATPlayerController::InputKey(const FInputKeyParams& params)
{
   bool result = Super::InputKey(params);

   // TODO: TAT Input stuff

   return result;
}

bool ATATPlayerController::IsMoveInputIgnored() const
{
   bool isIgnored = Super::IsMoveInputIgnored();
   if (!isIgnored)
   {
      if (const IGameplayTagAssetInterface* characterTags = GetPawn<IGameplayTagAssetInterface>())
      {
         isIgnored |= characterTags->HasMatchingGameplayTag(UTATProjectSettings::Get().MovementDisabledStatusTag);
      }
      if (ATATPlayerState* ps = GetTATPlayerState())
      {
         isIgnored |= ps->GetMapIntroState() != ETATMapIntroState::Complete;
      }
   }
   return isIgnored;
}

bool ATATPlayerController::IsLookInputIgnored() const
{
   bool isIgnored = Super::IsLookInputIgnored();
   if (!isIgnored)
   {
      if (const IGameplayTagAssetInterface* characterTags = GetPawn<IGameplayTagAssetInterface>())
      {
         isIgnored |= characterTags->HasMatchingGameplayTag(UTATProjectSettings::Get().LookDisabledStatusTag);
      }
      if (ATATPlayerState* ps = GetTATPlayerState())
      {
         isIgnored |= ps->GetMapIntroState() != ETATMapIntroState::Complete;
      }
   }
   return isIgnored;
}

// Potentially we will want to combine UpdateHiddenActors and this function so that we only have to go through all of the hidden actors
// and their components once. In order to save us some performance costs.
void ATATPlayerController::UpdateHiddenComponents(const FVector& viewLocation, TSet<FPrimitiveComponentId>& hiddenComponents)
{
   // Let the thief vision subsystem add any components it wants to be hidden
   if (IsLocalPlayerController())
   {
      if (UTATThiefVisionSubsystem* thiefVisionSubsystem = GetWorld()->GetSubsystem<UTATThiefVisionSubsystem>())
      {
         thiefVisionSubsystem->ClientGetHiddenThiefVisionComponents(this, viewLocation, hiddenComponents);
      }
   }
}

void ATATPlayerController::AuthoritySetRespawnPoint(AActor* newRespawnPoint)
{
   check(HasAuthority());
   _respawnPointActor = newRespawnPoint;
   _RefreshRespawnMarkerLocation();
}

void ATATPlayerController::AuthorityAddRespawnArea(ATATRespawnAreaOverlapVolume* respawnAreaOverlapVolume)
{
   check(HasAuthority());
   _currentRespawnAreaOverlapVolumes.AddUnique(respawnAreaOverlapVolume);
   
   respawnAreaOverlapVolume->OnAIDetectedWithinSafeZoneChanged.AddUniqueDynamic(
      this, &ThisClass::_AuthorityAIDetectedWithinRespawnSafeZone);
   
   _currentRespawnAreaOverlapVolumes.Sort(
      [](
         const TObjectPtr<ATATRespawnAreaOverlapVolume>& a, 
         const TObjectPtr<ATATRespawnAreaOverlapVolume>& b)
   {
      return a->GetPriority() < b->GetPriority();
   });
   _RefreshRespawnMarkerLocation();
}

void ATATPlayerController::AuthorityRemoveRespawnArea(ATATRespawnAreaOverlapVolume* respawnAreaOverlapVolume)
{
   check(HasAuthority());
   if (const AOSECharacterBase* character = GetPawn<AOSECharacterBase>())
   {
      // Only remove respawn areas if we aren't uncon. When we start "laying down" we turn of all of our overlap eventsf
      // via TATCollisionUtils::SetOverlayForLyingDown which removes us from any areas we're already in.
      // Instead, we clear the list when we respawn just before we re-enable our collision
      if (character->IsLyingDown())
      {
         return;
      }
   }
   respawnAreaOverlapVolume->OnAIDetectedWithinSafeZoneChanged.RemoveAll(this);
   _currentRespawnAreaOverlapVolumes.Remove(respawnAreaOverlapVolume);
   _RefreshRespawnMarkerLocation();
}

void ATATPlayerController::AuthorityClearRespawnAreas()
{
   check(HasAuthority());
   for (TObjectPtr<ATATRespawnAreaOverlapVolume>& volume : _currentRespawnAreaOverlapVolumes)
   {
      volume->OnAIDetectedWithinSafeZoneChanged.RemoveAll(this);
   }
   _currentRespawnAreaOverlapVolumes.Empty();
   _RefreshRespawnMarkerLocation();
}

ATATRespawnAreaSpawnLocation* GetFirstValidPlayerStartFromCurrentRespawnVolumes(
   const TArray<TObjectPtr<ATATRespawnAreaOverlapVolume>>& volumes,
   const APawn* pawn,
   TObjectPtr<ATATRespawnAreaOverlapVolume>& validVolume)
{
   for (const TObjectPtr<ATATRespawnAreaOverlapVolume>& overlapVolume : volumes)
   {
      if (ATATRespawnAreaSpawnLocation* bestSpawn = overlapVolume->GetValidPlayerStartForVolume(pawn))
      {
         validVolume = overlapVolume;
         return bestSpawn;
      }
   }
   return nullptr;
}

AActor* ATATPlayerController::GetRespawnPoint() const
{
   if (_currentRespawnAreaOverlapVolumes.Num() > 0)
   {
      // This list _should_ be in priority order. So loop over until we find a valid spawn location.
      TObjectPtr<ATATRespawnAreaOverlapVolume> bestVolume;
      ATATRespawnAreaSpawnLocation* bestSpawn = GetFirstValidPlayerStartFromCurrentRespawnVolumes(_currentRespawnAreaOverlapVolumes, GetPawn(), bestVolume);
      if (bestSpawn)
      {
         return bestSpawn;
      }
   }
   return _respawnPointActor;
}

void ATATPlayerController::BeginTempSpectating()
{
   if (HasAuthority())
   {
      ServerBeginTempSpectating_Implementation();
   }
   else
   {
      ServerBeginTempSpectating();
   }
}

void ATATPlayerController::EndTempSpectating()
{
   if (HasAuthority())
   {
      ServerEndTempSpectating_Implementation();
   }
   else
   {
      ServerEndTempSpectating();
   }
}

void ATATPlayerController::ServerBeginTempSpectating_Implementation()
{
   check(HasAuthority());

   auto* currentPawn = GetPawn();
   if (_pawnCachedWhileSpectating == nullptr &&
       currentPawn != nullptr)
   {
      _pawnCachedWhileSpectating = currentPawn;
      _pawnCachedWhileSpectating->UnPossessed();
   }

   // start spectating
   {
      ChangeState(NAME_Spectating);
      if (auto* ps = GetTATPlayerState())
      {
         ps->SetIsSpectator(true);
      }
      bPlayerIsWaiting = true;
   }

   ClientGotoState(NAME_Spectating);
}

void ATATPlayerController::ServerEndTempSpectating_Implementation()
{
   check(HasAuthority());

   if (_pawnCachedWhileSpectating)
   {
      Possess(_pawnCachedWhileSpectating);
   }
   else
   {
      if (AGameModeBase* gameMode = GetWorld()->GetAuthGameMode())
      {
         gameMode->RestartPlayer(this);
      }
   }
   _pawnCachedWhileSpectating = nullptr;
}

void ATATPlayerController::BeginSpectatingState()
{
   Super::BeginSpectatingState();
   OnSpectatingStateChanged.Broadcast(true);
}

void ATATPlayerController::EndSpectatingState()
{
   Super::EndSpectatingState();
   OnSpectatingStateChanged.Broadcast(false);
   SetPlayerCameraMode(EPlayerCameraMode::Default);
}

void ATATPlayerController::_AuthorityAIDetectedWithinRespawnSafeZone(
   ATATRespawnAreaOverlapVolume* volume,
   bool bDetected)
{
   TObjectPtr<ATATRespawnAreaOverlapVolume> bestVolume;
   GetFirstValidPlayerStartFromCurrentRespawnVolumes(_currentRespawnAreaOverlapVolumes, GetPawn(), bestVolume);
   
   if (IsValid(bestVolume))
   {
      _aiDetectedInCurrentBestRespawnArea = bestVolume->IsAIDetectedWithinSafeZone();
      _HandleAIDetectedInCurrentBestRespawnAreaChanged();
   }
}

#if (OSE_CHEATS_ENABLED)

void ATATPlayerController::_TATCheat_ChangeCharacter(ETATCharacter character)
{
   if (ATATPlayerState* ps = GetTATPlayerState())
   {
      ps->ChangeTATCharacter(character);
   }
}

void ATATPlayerController::_TATCheat_CameraMode_Toggle(const EPlayerCameraMode newMode)
{
   const EPlayerCameraMode oldMode = GetPlayerCameraMode();
   if ((newMode != oldMode) && (newMode != EPlayerCameraMode::Default))
   {
      SetPlayerCameraMode(newMode);
   }
   else
   {
      ResetPlayerCameraMode();
   }
}

void ATATPlayerController::_TATCheat_CameraMode_Toggle1P()
{
   _TATCheat_CameraMode_Toggle(EPlayerCameraMode::FirstPerson);
}

void ATATPlayerController::_TATCheat_CameraMode_Toggle3P()
{
   _TATCheat_CameraMode_Toggle(EPlayerCameraMode::ThirdPerson);
}

#endif //OSE_CHEATS_ENABLED

void ATATPlayerController::_OnEnterFirstPersonCameraTimeout()
{
   SetPlayerCameraMode(EPlayerCameraMode::FirstPerson);
}

void ATATPlayerController::_TrySpawnScreenMgr()
{
   if (!_screenMgr)
   {
      UE_LOG(LogTATPlayerController, Verbose, TEXT("Spawned ScreenMgr"));
      _screenMgr = NewObject<UTATScreenMgr>(this, TEXT("ScreenManager"));
      check(_screenMgr);
      _screenMgr->RegisterComponent();
      OnTATScreenMgrSpawned.Broadcast(_screenMgr);
   }
}

void ATATPlayerController::_TickLocalSubsystemState()
{
   if (IsLocalController())
   {
      bool isUsingMonocular = false;
      bool isAstrallyProjecting = false;

      if (const IAbilitySystemInterface* asi = Cast<IAbilitySystemInterface>(GetPawn()))
      {
         if (const UAbilitySystemComponent* asc = asi->GetAbilitySystemComponent())
         {
            const UTATProjectSettings& settings = UTATProjectSettings::Get();

            isUsingMonocular = asc->HasMatchingGameplayTag(settings.UsingMonocularTag);
            isAstrallyProjecting = asc->HasMatchingGameplayTag(settings.AstralProjectionTag);
         }
      }

      if (UTATAIStateWorldSubsystem* aiStateSubsystem = GetWorld()->GetSubsystem<UTATAIStateWorldSubsystem>())
      {
         aiStateSubsystem->SetLocalPlayerIsUsingMonocular(isUsingMonocular);
         aiStateSubsystem->SetLocalPlayerIsStandingStill(_isStandingStill);
      }

      if (auto* localPlayerStateSubsystem = GetWorld()->GetSubsystem<UTATLocalPlayerStateWorldSubsystem>())
      {
         // Any future properties on UTATLocalPlayerStateWorldSubsystem should be set here
      }
   }
}

void ATATPlayerController::_TickPlayerMovementState(const UCharacterMovementComponent* characterMovementComponent)
{
   check(IsValid(characterMovementComponent));

   const float currentTime = GetWorld()->GetTimeSeconds();
   const float currentMoveSpeedSqr = characterMovementComponent->GetLastUpdateVelocity().SizeSquared();
   const float movementThresholdSqr = _standingStillMovementEpsilon * _standingStillMovementEpsilon;

   // If moving, cache _lastMovedTime
   const bool wasStandingStill = _isStandingStill;
   if (currentMoveSpeedSqr > movementThresholdSqr)
   {
      _isStandingStill = false;
      _mostRecentMovedTime = currentTime;
   }
   // Otherwise wait until we've been stationary for required duration before considering ourselves stationary
   else
   {
      const float timeSinceLastMoved = currentTime - _mostRecentMovedTime;
      if (timeSinceLastMoved >= _standingStillRequiredDuration)
      {
         _isStandingStill = true;
      }
   }

   // Apply/remove tag on change (for local player only)
   if (wasStandingStill != _isStandingStill && IsLocalPlayerController())
   {
      if (const APlayerState* ps = GetPlayerState<APlayerState>())
      {
         UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ps);
         check(asc);
         if (_isStandingStill)
         {
            asc->SetLooseGameplayTagCount(_standingStillTag, 1);
         }
         else
         {
            asc->SetLooseGameplayTagCount(_standingStillTag, 0);
         }
      }
   }
}

void ATATPlayerController::_OnRep_ThiefVisionIndicators()
{
   _OnThiefVisionIndicatorsUpdated();
}

void ATATPlayerController::_OnRep_AIDetectedInCurrentBestRespawnArea()
{
   _HandleAIDetectedInCurrentBestRespawnAreaChanged();
}

void ATATPlayerController::_HandleAIDetectedInCurrentBestRespawnAreaChanged()
{
   OnAIDetectedInCurrentBestRespawnAreaChanged.Broadcast(_aiDetectedInCurrentBestRespawnArea);
}

void ATATPlayerController::_OnThiefVisionIndicatorsUpdated()
{

}

UTATEnhancedInputComponent* ATATPlayerController::_GetEnhancedInputComponent() const
{
   if (!IsLocalPlayerController())
   {
      UE_LOG(LogTATPlayerController, Error, TEXT("[%s] | _GetEnhancedInputComponent() called on non-locally controlled player!"), *GetName());
      return nullptr;
   }
   const APawn* pawn = GetPawn();
   if (!pawn)
   {
      UE_LOG(LogTATPlayerController, Warning, TEXT("[%s] | _GetEnhancedInputComponent() called without possessed pawn to pull input component from!"), *GetName());
      return nullptr;
   }
   return CastChecked<UTATEnhancedInputComponent>(pawn->InputComponent);
}

void ATATPlayerController::_OnRep_RespawnPoint()
{
   if(IsLocalController())
   {
      _RefreshRespawnMarkerLocation();
   }
}

void ATATPlayerController::_OnRep_RespawnAreas()
{
   if(IsLocalController())
   {
      _RefreshRespawnMarkerLocation();
   }
}

void ATATPlayerController::_SpawnRespawnMarker()
{
   check(IsLocalController());
   if(_respawnPointMarkerClass.IsNull())
   {
      return;
   }
   
   const ATATWorldSettings& worldSettings = ATATWorldSettings::Get(this);
   if(worldSettings.MapType != ETATMapType::Mission)
   {
      return;
   }
   
   _respawnPointMarkerClass.ToSoftObjectPath().LoadAsync(FLoadSoftObjectPathAsyncDelegate::CreateWeakLambda(this, [this](const FSoftObjectPath&, UObject*)
   {
      if(_respawnPointMarkerClass.IsValid())
      {
         const FVector location = _CalculateRespawnMarkerLocation();
         _respawnPointMarker = GetWorld()->SpawnActor(_respawnPointMarkerClass.Get(), &location);
      }
   }));
}

void ATATPlayerController::_RefreshRespawnMarkerLocation()
{
   if(_respawnPointMarker)
   {
      _respawnPointMarker->SetActorLocation(_CalculateRespawnMarkerLocation());
   }
}

FVector ATATPlayerController::_CalculateRespawnMarkerLocation()
{
   for(const ATATRespawnAreaOverlapVolume* volume : _currentRespawnAreaOverlapVolumes)
   {
      if(IsValid(volume))
      {
         return volume->GetRespawnMarkerLocation();
      }
   }

   if(_respawnPointActor)
   {
      if(const ITATRespawnMarkerLocationInterface* markerProxy = Cast<ITATRespawnMarkerLocationInterface>(_respawnPointActor))
      {
         return markerProxy->GetRespawnMarkerLocation();
      }

      // Use offset for non-interface so play-from-here does not start offset
      return _respawnPointActor->GetActorLocation() + FVector(0, 0, UTATProjectSettings::Get().PlayerStartRespawnMarkerZOffset);
   }

   return FVector::ZeroVector;
}
