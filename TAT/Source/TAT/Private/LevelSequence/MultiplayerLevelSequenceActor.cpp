// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "LevelSequence/MultiplayerLevelSequenceActor.h"

// tat
#include "Online/TATGameState.h"
#include "Player/TATPlayerController.h"
#include "Player/TATPlayerState.h"
#include "UI/TATCinematicOverlayScreen.h"

// ue4
#include "Net/UnrealNetwork.h"
#include "LevelSequence.h"
#include "LevelSequencePlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MultiplayerLevelSequenceActor)

DEFINE_LOG_CATEGORY_STATIC(LogTATMultiplayerLevelSequenceActor, Log, All);

ATATMultiplayerLevelSequenceActor::ATATMultiplayerLevelSequenceActor(const FObjectInitializer& init)
   : Super(init)
{
   // cinematics are always relevant
   bAlwaysRelevant = true;

   // this should always tick, even on a dedicated server, so we can check for cutscene skipping
   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = true;
   PrimaryActorTick.bAllowTickOnDedicatedServer = true;
}

/* static */
ULevelSequencePlayer* ATATMultiplayerLevelSequenceActor::TATCreateLevelSequencePlayer(UObject* worldContextObject, const FTATMultiplayerLevelSequenceCreateParams& params, ATATMultiplayerLevelSequenceActor*& outActor)
{
   // logic mostly lifted /modified from ULevelSequencePlayer::CreateLevelSequencePlayer()
   if (!params.LevelSequence)
      return nullptr;

   UWorld* world = GEngine->GetWorldFromContextObject(worldContextObject, EGetWorldErrorMode::LogAndReturnNull);
   if (world == nullptr || world->bIsTearingDown)
      return nullptr;

   FActorSpawnParameters spawnParams;
   spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
   spawnParams.ObjectFlags |= RF_Transient;
   spawnParams.bAllowDuringConstructionScript = true;

   // Defer construction for autoplay so that BeginPlay() is called
   spawnParams.bDeferConstruction = true;

   ATATMultiplayerLevelSequenceActor* actor = world->SpawnActor<ATATMultiplayerLevelSequenceActor>(spawnParams);

   actor->PlaybackSettings = params.PlaybackSettings;
   actor->CameraSettings = params.CameraSettings;
   
   actor->LevelSequenceAsset = params.LevelSequence;
   actor->InitializePlayer();

   // the engine classes don't actually replicate a hard pointer to the level sequence which
   // is super awkward and the primary reason this game-side wrapper class exists
   actor->_levelSequence = params.LevelSequence;

   // oh and they also don't replicate the Playback and Camera Settings params so let's do that ourselves too.
   // Playback settings DO get set into the sequencer, but since we need to call Initialize() on the client ourselves
   // we actually need to have the correct set at that point, so we might as well replicate it all ourselves
   actor->_params = params;
   actor->_params.Configured = true;
   actor->_initialized = true;

   outActor = actor;

   const bool isServer = actor->_IsServer();

   // set whether this is going to be a local cinematic or replicated to all clients
   if (isServer)
   {
      // replicated or not
      actor->SetReplicatePlayback(params.Replicate);
      actor->bReplicates = params.Replicate;
      UE_LOG(LogTATMultiplayerLevelSequenceActor, Verbose, TEXT("Level sequence %s initialized on the server and %s replicate."), 
         *params.LevelSequence->GetName(),
         params.Replicate ? TEXT("will") : TEXT("will NOT"));

      // reset ready check for skips
      actor->_AuthorityResetReadyForCutsceneChecks();
   }
   else if (params.Replicate)
   {
      UE_LOG(LogTATMultiplayerLevelSequenceActor, Error, TEXT("Trying to spawn a multiplayer replicated level sequence %s from a client!  This will only run on the calling client!"), *params.LevelSequence->GetName());
   }
   else
   {
      UE_LOG(LogTATMultiplayerLevelSequenceActor, Verbose, TEXT("Local level sequence %s initialized."), *params.LevelSequence->GetName());
   }

   actor->FinishSpawning(FTransform());

   // we should know when it's over
   actor->_BindToOnSequenceFinished();

   // and now that we're playing the LS, possibly show an overlay
   actor->_TryInitializeCinematicOverlay();

   return actor->GetSequencePlayer();
}

/* static */
ATATMultiplayerLevelSequenceActor* ATATMultiplayerLevelSequenceActor::CreateLevelSequencePlayer(UObject* worldContextObject, const FTATMultiplayerLevelSequenceCreateParams& params)
{
   ATATMultiplayerLevelSequenceActor* outActor = nullptr;
   ULevelSequencePlayer* player = TATCreateLevelSequencePlayer(worldContextObject, params, outActor);
   return outActor;
}

void ATATMultiplayerLevelSequenceActor::BeginPlay()
{
   Super::BeginPlay();
}

void ATATMultiplayerLevelSequenceActor::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   Super::EndPlay(endPlayReason);
   _TryCleanUpCinematicOverlay();
}

void ATATMultiplayerLevelSequenceActor::Tick(float deltaSeconds)
{
   Super::Tick(deltaSeconds);
   _TryInitializeLevelSequence();
   _AuthorityCheckForSequenceSkip();
}

void ATATMultiplayerLevelSequenceActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
   DOREPLIFETIME_CONDITION(ATATMultiplayerLevelSequenceActor, _levelSequence, COND_InitialOnly);
   DOREPLIFETIME_CONDITION(ATATMultiplayerLevelSequenceActor, _params, COND_InitialOnly);
}

void ATATMultiplayerLevelSequenceActor::_OnRep_LevelSequence()
{
   _TryInitializeLevelSequence();
}

void ATATMultiplayerLevelSequenceActor::_OnRep_Params()
{
   _TryInitializeLevelSequence();
}

void ATATMultiplayerLevelSequenceActor::_BindToOnSequenceFinished()
{
   if (ULevelSequencePlayer* sequencePlayer = GetSequencePlayer())
   {
      sequencePlayer->OnFinished.AddUniqueDynamic(this, &ATATMultiplayerLevelSequenceActor::_OnSequencePlayerFinished);
      sequencePlayer->OnStop.AddUniqueDynamic(this, &ATATMultiplayerLevelSequenceActor::_OnSequencePlayerFinished);
   }
}

void ATATMultiplayerLevelSequenceActor::_TryInitializeLevelSequence()
{
   if (!_initialized)
   {
      // don't kick off the cinematic until we have our local player controller.  cinematics can control movement
      // input and the hud, all of which need the local pc to exist first.
      ATATPlayerController* localPC = ATATPlayerController::GetLocalTATPlayerController(this);
      if (localPC && _levelSequence && _params.Configured)
      {
         ULevelSequencePlayer* sequencePlayer = GetSequencePlayer();
         check(sequencePlayer);
         sequencePlayer->SetPlaybackSettings(_params.PlaybackSettings);
         sequencePlayer->Initialize(_levelSequence, GetLevel(), _params.CameraSettings);
         _initialized = true;
         UE_LOG(LogTATMultiplayerLevelSequenceActor, Verbose, TEXT("Initialized replicated level sequence %s"), *_levelSequence->GetName());

         // and now that we're playing the LS, possibly show an overlay
         _TryInitializeCinematicOverlay();
      }
   }
}

void ATATMultiplayerLevelSequenceActor::_TryInitializeCinematicOverlay()
{
   ATATPlayerController* localPC = ATATPlayerController::GetLocalTATPlayerController(this);
   if (localPC && 
       !_overlayScreen && 
       _params.CinematicOverlayParams.ShowCinematicOverlay && 
       _params.CinematicOverlayParams.OverlayScreenClass)
   {
      _overlayScreen = CreateWidget<UTATCinematicOverlayScreen>(localPC, _params.CinematicOverlayParams.OverlayScreenClass);
      if (_overlayScreen)
      {
         _overlayScreen->CinematicSetup(_params.CinematicOverlayParams.AllowSequenceSkip);
      }
   }
}

void ATATMultiplayerLevelSequenceActor::_TryCleanUpCinematicOverlay()
{
   if (_overlayScreen)
   {
      _overlayScreen->CinematicComplete();
      _overlayScreen = nullptr;
   }
}

void ATATMultiplayerLevelSequenceActor::_AuthorityResetReadyForCutsceneChecks()
{
   if (_IsServer())
   {
      ATATGameState* gs = ATATGameState::GetTATGameState(this);
      check(gs);
      gs->AuthorityResetAllPlayersReadyForCutsceneSkip();
   }
}

void ATATMultiplayerLevelSequenceActor::_AuthorityCheckForSequenceSkip()
{
   ULevelSequencePlayer* sequencePlayer = GetSequencePlayer();
   if (_IsServer() && sequencePlayer && _params.CinematicOverlayParams.AllowSequenceSkip)
   {
      // re-using the readycheck system to mean skip, in this case
      int numPlayersReady = 0;
      ATATGameState* gs = ATATGameState::GetTATGameState(this);
      check(gs);

      // stop the sequence player!
      if (gs->GetAreAllPlayersReadyForCutsceneSkip())
      {
         sequencePlayer->Stop();

         // reset for next time since this is global state
         _AuthorityResetReadyForCutsceneChecks();
      }
   }
}

void ATATMultiplayerLevelSequenceActor::_OnSequencePlayerFinished()
{
   if (_levelSequence)
   {
      UE_LOG(LogTATMultiplayerLevelSequenceActor, Verbose, TEXT("Sequence %s finished"), *_levelSequence->GetName());
      
      // when we have authority, optionally clean up this actor when the sequence is complete
      if (HasAuthority() && _params.DestroyWhenComplete)
      {
         UE_LOG(LogTATMultiplayerLevelSequenceActor, Verbose, TEXT("Destroying %s because the sequence completed"), *GetName());
         Destroy();
      }

      // if we spawned an overlay screen, let it can clean itself up now
      _TryCleanUpCinematicOverlay();
   }
}

bool ATATMultiplayerLevelSequenceActor::_IsServer() const
{
   if (AGameStateBase* gs = GetWorld()->GetGameState())
      return gs->HasAuthority();
   return false;
}

