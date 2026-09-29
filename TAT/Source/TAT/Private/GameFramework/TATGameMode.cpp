// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "GameFramework/TATGameMode.h"

// tat
#include "Character/TATTeams.h"
#include "Developer/TATEditorSettings.h"
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATAuthMapSeedSubsystem.h"
#include "GameFramework/TATPlayerStart.h"
#include "GameFramework/TATWorldSettings.h"
#include "GameFramework/SafeRoom/TATSafeRoomPlayerStart.h"
#include "Online/TATGameSession.h"
#include "Online/TATGameState.h"
#include "Player/TATPlayerController.h"
#include "Player/TATPlayerState.h"
#include "Player/TATCharacter.h"
#include "Player/TATSpectatorPawn.h"
#include "UI/TATHUD.h"
#include "UI/TATToastBroadcaster.h"
#include "Settings/TATMatchSettingsPropertyDef.h"
#include "TATGameInstance.h"

// ose
#include "Utl/OSEUtlFunctionLibrary.h"

// ose dedicated server
#include "ServerManager/OSEDedicatedServerManagerBase.h"

// ue4
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/PlayerStartPIE.h"
#include "GameFramework/PlayerStart.h"
#include "Math/Color.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameMode)

DEFINE_LOG_CATEGORY_STATIC(LogTATGameMode, Log, All);

namespace GameModeHelpers
{
   // Just to make the template work
   static APlayerStart* ResolvePlayerStart(APlayerStart* playerStart) { return playerStart; }
   static APlayerStart* ResolvePlayerStart(TWeakObjectPtr<APlayerStart> playerStart) { return playerStart.Get(); }
}

ATATGameMode::ATATGameMode()
   : Super()
{
   // Use our custom classes
   DefaultPawnClass = ATATCharacter::StaticClass();
   GameSessionClass = ATATGameSession::StaticClass();
   GameStateClass = ATATGameState::StaticClass();
   HUDClass = ATATHUD::StaticClass();
   PlayerControllerClass = ATATPlayerController::StaticClass();
   PlayerStateClass = ATATPlayerState::StaticClass();
   SpectatorClass = ATATSpectatorPawn::StaticClass();
}

void ATATGameMode::StartPlay()
{
   _TryInitRandomness();
   Super::StartPlay();
}

void ATATGameMode::BeginPlay()
{
   Super::BeginPlay();

   ATATWorldSettings& worldSettings = ATATWorldSettings::Get(GetWorld());

   // spawn a toast broadcaster actor to serve as a dedicated replication channel for sending toast messages to players
   if (worldSettings.MapType != ETATMapType::Menu && worldSettings.MapType != ETATMapType::Transition)
   {
      UClass* broadcasterClass = _toastBroadcasterClass;
      if (!broadcasterClass)
      {
         broadcasterClass = ATATToastBroadcaster::StaticClass();
      }
      FActorSpawnParameters toastBroadcasterSpawnParams;
      toastBroadcasterSpawnParams.bNoFail = true;
      _toastBroadcaster = GetWorld()->SpawnActor<ATATToastBroadcaster>(broadcasterClass, toastBroadcasterSpawnParams);
   }

   // activate a dedicated server once we've loaded into the hub

   if ((worldSettings.MapType == ETATMapType::Hub || worldSettings.MapType == ETATMapType::ServerStart) && IsNetMode(NM_DedicatedServer))
   {
      UOSEDedicatedServerManagerBase& serverMgr = UOSEDedicatedServerManagerBase::Get(*this);
      serverMgr.ServerReadyToAcceptPlayers();
   }
}

void ATATGameMode::PostInitializeComponents()
{
   Super::PostInitializeComponents();

#if OSE_CHEATS_ENABLED
   if (UTATGameInstance* gameInstance = UTATGameInstance::GetTATGameInstance(this))
   {
      // If we have any default match settings to apply, do it now
      // Running it here rather than BeginPlay so that it is initialized before player starts are needed
      UTATEditorSettings::Get().ApplyDefaultMatchSettings(gameInstance->GetMatchSettings(), FTATMatchSettingsQueryContext::MakeFromWorldContext(this));
      gameInstance->AuthorityNotifyCheatUpdatedMatchSettings();
   }
#endif
}

bool ATATGameMode::AllowCheats(APlayerController* pc)
{
   if (GetNetMode() == NM_Standalone || GIsEditor)
   {
      return true;
   }

#if UE_BUILD_DEVELOPMENT   
   return true;//dev builds allow cheating across the board
#else
   return false;
#endif//UE_BUILD_DEVELOPMENT
}

void ATATGameMode::PreLogin(const FString& options, const FString& address, const FUniqueNetIdRepl& uniqueId, FString& errorMessage)
{
   UTATGameInstance* tatGameInstance = GetWorld()->GetGameInstance<UTATGameInstance>();
   const bool isLateJoin = tatGameInstance != nullptr && tatGameInstance->HasPartyMembers() && !tatGameInstance->IsPlayerInParty(uniqueId);
   if (isLateJoin)
   {
      UE_LOG(LogTATGameMode, Log, TEXT("PreLogin: player late-join (options=%s, address=%s, uniqueId=%s)"),
         *options, *address, *uniqueId.ToString());

      // If needed, we can reject player login by setting an error message if they are not already in the party created from the menu flow.
      // Example:
      //#if !WITH_EDITOR
      //errorMessage = TEXT("Player not in party");
      //#endif
   }

   if (errorMessage.IsEmpty())
   {
      // accept/reject players by setting the error string
      if (IsNetMode(NM_DedicatedServer))
      {
         UOSEDedicatedServerManagerBase& serverMgr = UOSEDedicatedServerManagerBase::Get(*this);
         serverMgr.PreLogin(options, address, uniqueId, errorMessage);
      }
   }

   // Call ApproveLogin directly rather than relying on the parent since it requires that the client and server OSS match
   // and will error out with incompatible_unique_net_id
   // NB. Super::PreLogin handles server capacity checks
   if (errorMessage.IsEmpty())
   {
      errorMessage = GameSession->ApproveLogin(options);
      FGameModeEvents::GameModePreLoginEvent.Broadcast(this, uniqueId, errorMessage);
   }

   if (!errorMessage.IsEmpty())
   {
      UE_LOG(LogTATGameMode, Error, TEXT("PreLogin() Error: %s"), *errorMessage);
      return;
   }

   // Track this player's unique id until they're fully logged in (eg. have an assigned player controller) so that we can count the number of pending logins
   _pendingPlayerLogins.AddUnique(uniqueId);

   // If the player hasn't completed the login within a few seconds, assume they've disconnected or otherwise left the login process
   const float playerLoginTimeoutSeconds = 5.0f;
   FTimerHandle handle;
   TWeakObjectPtr<ATATGameMode> weakThis = this;
   GetWorldTimerManager().SetTimer(handle, [weakThis, uniqueId]()
      {
         if (ATATGameMode* self = weakThis.Get())
         {
            const int32 playerTimedOut = self->_pendingPlayerLogins.RemoveSingleSwap(uniqueId);
            if (playerTimedOut > 0)
            {
               UE_LOG(LogTATGameMode, Warning, TEXT("Player login timed out (%s)"), *uniqueId.ToString());
            }
         }
      }, playerLoginTimeoutSeconds, false);
}

APlayerController* ATATGameMode::Login(UPlayer* newPlayer, ENetRole inRemoteRole, const FString& portal, const FString& options, const FUniqueNetIdRepl& uniqueId, FString& errorMessage)
{
   // Player has logged in successfully, so remove them from the list of pending players to free up a slot for them.
   // This *MUST* happen before calling Super::Login, because that will do a server capacity check.
   // See also: ATATGameSession::AtCapacity()
   _pendingPlayerLogins.RemoveSingleSwap(uniqueId);

   APlayerController* pc = Super::Login(newPlayer, inRemoteRole, portal, options, uniqueId, errorMessage);

   if (pc && IsNetMode(NM_DedicatedServer))
   {
      UOSEDedicatedServerManagerBase& serverMgr = UOSEDedicatedServerManagerBase::Get(*this);
      serverMgr.Login(options, uniqueId, errorMessage);
   }

   return pc;
}

void ATATGameMode::_TryAssignTeam(APlayerController* newPlayer)
{
   ATATPlayerState* ps = newPlayer->GetPlayerState<ATATPlayerState>();
   if (ps && ps->GetTeam() == IOSETeamInterface::kInvalidTeam)
   {
      ps->AuthoritySetTeam(_GetTeamIndexForNewPlayer(newPlayer));
   }
}

void ATATGameMode::PostLogin(APlayerController* newPlayer)
{
   Super::PostLogin(newPlayer);
   
   check(newPlayer);
   _TryAssignTeam(newPlayer);
}

void ATATGameMode::Logout(AController* exitingController)
{
   Super::Logout(exitingController);

   if (IsNetMode(NM_DedicatedServer))
   {
      // The subsystem may have been torn down already when ending PIE (as of 5.4)
      if (UOSEDedicatedServerManagerBase* serverMgr = UOSEDedicatedServerManagerBase::TryGet(*this))
      {
         serverMgr->Logout(exitingController);
      }
   }
}

void ATATGameMode::GetSeamlessTravelActorList(bool toTransition, TArray<AActor*>& actorList)
{
   Super::GetSeamlessTravelActorList(toTransition, actorList);

   // server can add actors to seamless travel here
   for (FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator(); it; ++it)
   {
      APlayerController* pc = it->Get();
      actorList.AddUnique(pc);
      if (APawn* pawn = pc->GetPawn())
      {
         actorList.AddUnique(pawn);
      }
   }
}

void ATATGameMode::HandleSeamlessTravelPlayer(AController*& pc)
{
   Super::HandleSeamlessTravelPlayer(pc);
}

AActor* ATATGameMode::ChoosePlayerStart_Implementation(AController* controller)
{
   // We may get here before StartPlay() without randomness initialized, so lazy-init it here before use.
   _TryInitRandomness();

   // Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
   for (TActorIterator<APlayerStartPIE> it(GetWorld()); it; ++it)
   {
      APlayerStartPIE* playerStartPIE = (*it);
      return playerStartPIE;
   }

   // try to find one for the team
   if (AActor* teamStart = _ChoosePlayerStartForTeam(controller))
   {
      return teamStart;
   }

   // otherwise use our modified ue4 default implementation
   return _ChoosePlayerStartDefault(controller);
}

bool ATATGameMode::UpdatePlayerStartSpot(AController* player, const FString& portal, FString& outErrorMessage)
{
   const bool result = Super::UpdatePlayerStartSpot(player, portal, outErrorMessage);
   if (result)
   {
      ATATPlayerController* pc = Cast<ATATPlayerController>(player);
      AActor* startSpot = player->StartSpot.Get();
      if (pc && startSpot)
      {
         pc->AuthoritySetRespawnPoint(startSpot);
      }
   }
   
   return result;
}

FLinearColor ATATGameMode::GetColorForTeamID(const uint8 teamID)
{
   const FLinearColor* colorForTeamID = _teamIDToColor.Find(teamID);
   if(colorForTeamID == nullptr)
   {
      // Get pool of player colors from project settings
      const UTATProjectSettings& settings = UTATProjectSettings::Get();
      if(settings.PlayerColorPool.IsValidIndex(_nextTeamColorID))
      {
         const FLinearColor nextColor =  settings.PlayerColorPool[_nextTeamColorID];
         _teamIDToColor.Add(teamID, nextColor);
         _nextTeamColorID++;
         return nextColor;
      }
      UE_LOG(LogTATGameMode, Error, TEXT("%s PickPlayerColorFromPool() - failed to find a color, using default"), *GetName());
      return FLinearColor::Black;
   }
   return *colorForTeamID;
}

uint8 ATATGameMode::_GetTeamIndexForNewPlayer(APlayerController* newPlayer)
{
   return UTATProjectSettings::GetTeamAssignmentForCharacterType(ETATTeamCharacterType::Player);
}

AActor* ATATGameMode::_ChoosePlayerStartForTeam(AController* controller)
{
   ATATPlayerState* ps = controller->GetPlayerState<ATATPlayerState>();
   if (ps == nullptr)
   {
      return nullptr;
   }

   _TryAssignTeam(CastChecked<APlayerController>(controller));

   uint8 teamId = ps->GetTeam();
   FPlayerStartBucket* bucket = _GetStartBucketForTeam(teamId);
   if (bucket == nullptr)
   {
      return nullptr;
   }

   // If there is an unused index available, use that
   if (bucket->PlayerStarts.IsValidIndex(bucket->CurrentIndex))
   {
      APlayerStart* playerStart = bucket->PlayerStarts[bucket->CurrentIndex].Get();
      bucket->CurrentIndex++;
      return playerStart;
   }

   // Otherwise try to find one with physical space?
   return _ChoosePlayerStart(controller, MakeConstArrayView(bucket->PlayerStarts));
}

AActor* ATATGameMode::_ChoosePlayerStartDefault(AController* controller)
{
   TArray<APlayerStart*> allPlayerStarts;
   UWorld* world = GetWorld();
   for (APlayerStart* playerStart : TActorRange<APlayerStart>(world))
   {

      if (playerStart->IsA<ATATPlayerStart>())
      {
         continue;
      }

      // exclude some safe room starts while they are still around
      if (ATATSafeRoomPlayerStart* safeRoomStart = Cast<ATATSafeRoomPlayerStart>(playerStart);
         safeRoomStart && !safeRoomStart->IsSafeRoomAvailableForInitialAssignment())
      {
         continue;
      }

      allPlayerStarts.Add(playerStart);
   }

   return _ChoosePlayerStart(controller, MakeConstArrayView(allPlayerStarts));
}

template<typename T>
AActor* ATATGameMode::_ChoosePlayerStart(AController* controller, TConstArrayView<T> allPlayerStarts)
{
   // NOTE: Intentionally not using GetRandomStream() in here, we want the results of using that object to be consistent across runs
   // so it helps us choose an area but not specific spawns within that area.  This is because we can't control the number of players
   // or which spots may be occupied during spawn time during player spawning for multi-run consistency.

   UWorld* world = GetWorld();
   UClass* pawnClass = GetDefaultPawnClassForController(controller);
   APawn* pawnToFit = pawnClass ? pawnClass->GetDefaultObject<APawn>() : nullptr;
   TArray<APlayerStart*> unoccupiedPlayerStarts;
   TArray<APlayerStart*> occupiedPlayerStarts;
   for (T playerStartHandle : allPlayerStarts)
   {
      APlayerStart* playerStart = GameModeHelpers::ResolvePlayerStart(playerStartHandle);
      if (playerStart == nullptr)
      {
         continue;
      }
      FVector actorLocation = playerStart->GetActorLocation();
      const FRotator actorRotation = playerStart->GetActorRotation();
      if (!world->EncroachingBlockingGeometry(pawnToFit, actorLocation, actorRotation))
      {
         unoccupiedPlayerStarts.Add(playerStart);
      }
      else if (world->FindTeleportSpot(pawnToFit, actorLocation, actorRotation))
      {
         occupiedPlayerStarts.Add(playerStart);
      }
   }

   APlayerStart* playerStart = nullptr;
   if (unoccupiedPlayerStarts.Num() > 0)
   {
      // TODO: Why are we doing this?
      if (GEngine->IsEditor())
      {
         playerStart = unoccupiedPlayerStarts[0];
      }
      else
      {
         playerStart = unoccupiedPlayerStarts[_randomStream.RandHelper(unoccupiedPlayerStarts.Num())];
      }
   }
   else if (occupiedPlayerStarts.Num() > 0)
   {
      // TODO: Why are we doing this?
      if (GEngine->IsEditor())
      {
         playerStart = occupiedPlayerStarts[0];
      }
      else
      {
         playerStart = occupiedPlayerStarts[_randomStream.RandHelper(occupiedPlayerStarts.Num())];
      }
   }
   return playerStart;
}

void ATATGameMode::_TryInitRandomness()
{
   if (!_isRandomnessInitialized)
   {
      _isRandomnessInitialized = true;
      _GenerateRandomStream();
   }
}

void ATATGameMode::_GenerateRandomStream()
{
   UTATAuthMapSeedSubsystem* seedSubsystem = GetWorld()->GetSubsystem<UTATAuthMapSeedSubsystem>();
   const int32 seed = seedSubsystem->GetOrCreateSeed();

   UE_LOG(LogTATGameMode, Log, TEXT("Game mode is spawning random world and mission elements with seed %d"), seed);
   _randomStream.Initialize(seed);

   // CONSIDER: should this push more directly now?
   if (auto* gameState = GetGameState<ATATGameState>())
   {
      gameState->AuthorityInitializeMapSeed(seed);
   }
}

void ATATGameMode::_GeneratePlayerStartAreaIndexes()
{
   if (_startingAreasInitialized)
   {
      return;
   }
   _startingAreasInitialized = true;
   
   _TryInitRandomness();
   for (ATATPlayerStart* playerStart : TActorRange<ATATPlayerStart>(GetWorld()))
   {
      _availableStartingAreas.AddUnique(playerStart->GetAreaIndex());
   }
   _availableStartingAreas.Sort();
   UOSEUtlFunctionLibrary::ShuffleWithRandomStream(_availableStartingAreas, _randomStream);
}

ATATGameMode::FPlayerStartBucket* ATATGameMode::_GetStartBucketForTeam(uint8 team)
{
   if (FPlayerStartBucket* found = _teamPlayerStartBuckets.Find(team))
   {
      return found;
   }
   
   _GeneratePlayerStartAreaIndexes();
   if (_availableStartingAreas.Num() > 0)
   {
      const int32 areaIndex = _availableStartingAreas.Pop(EAllowShrinking::No);
      FPlayerStartBucket bucket;
      for (ATATPlayerStart* playerStart : TActorRange<ATATPlayerStart>(GetWorld()))
      {
         if (playerStart->GetAreaIndex() == areaIndex)
         {
            bucket.PlayerStarts.Add(playerStart);
         }
      }

      return &_teamPlayerStartBuckets.Emplace(team, MoveTemp(bucket));
   }

   return nullptr;
}

