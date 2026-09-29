// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "GameFramework/TATTravelMgr.h"

// tat
#include "TATGameInstance.h"
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATWorldSettings.h"
#include "Variation/TATMapVariationMgrComponent.h"
#include "Online/TATGameState.h"
#include "Player/TATCharacter.h"
#include "Player/TATPlayerController.h"
#include "Player/TATPlayerState.h"
#include "UI/TATUIZOrder.h"

// ose
#include "Graphics/Performance/OSEPerformanceTestComponent.h"

// ue4
#include "MoviePlayer.h"
#include "ShaderPipelineCache.h"
#include "Blueprint/UserWidget.h"
#include "Engine/UserInterfaceSettings.h"
#include "GameFramework/GameModeBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTravelMgr)

DEFINE_LOG_CATEGORY_STATIC(LogTATTravelMgr, Log, All);

void UTATTravelMgr::Init(UTATGameInstance* instanceOwner)
{
   _gameInstance = instanceOwner;
   check(_gameInstance);

   // core delegates
   FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UTATTravelMgr::_OnPostLoadMapWithWorld);

   // TODO: async load after init or something?  doing the lazy thing for now...
   const UTATProjectSettings& projectSettings = *GetDefault<UTATProjectSettings>();
   if (!_gameInstance->IsDedicatedServerInstance())
   {
      // generic loading screen
      if (projectSettings.LoadingScreenWidget)
      {
         _loadingScreenUserWidget = CreateWidget<UUserWidget>(_gameInstance, projectSettings.LoadingScreenWidget.LoadSynchronous(), TEXT("TATTravelMgr_LoadingScreenWidget"));
         check(_loadingScreenUserWidget);
         _loadingScreenSlateWidget = _loadingScreenUserWidget->TakeWidget();
      }
   }

   // setup the movie player to display our loading widget
   // this is the callback we get whenever OpenLevel is called and we show our generic loading screen widget
   GetMoviePlayer()->OnPrepareLoadingScreen().AddUObject(this, &ThisClass::_SetupMoviePlayerForTravel);

   // initial OnWorldChanged call now that this mgr exists (game instance may have already loaded a world before we init'd)
   OnWorldChanged(nullptr, _gameInstance->GetWorld());
}

void UTATTravelMgr::OnWorldChanged(UWorld* oldWorld, UWorld* newWorld)
{
#if WITH_EDITOR
   // when playing as a listen server in the editor we don't get _OnPostLoadMapWithWorld() loading into the listen server, sadly.
   // it's kind of a shame because we get it everywhere else when running in a package build etc.  We're using this callback to
   // make this loading flow work in an editor listen server flow, too
   if (newWorld)
   {
      _OnPostLoadMapWithWorld(newWorld);
   }
#endif // WITH_EDITOR
   
   FGenericCrashContext::SetGameData(TEXT("MapName"), newWorld ? newWorld->GetMapName() : FString());
}

void UTATTravelMgr::Shutdown()
{
   _loadingScreenUserWidget = nullptr;
   _loadingScreenSlateWidget = nullptr;
}

/* static */
void UTATTravelMgr::ServerInitiateTravel(const UObject* contextObj, const FString& mapName, ETATTravelType travelType)
{
   UTATTravelMgr& travelMgrForWorld = UTATGameInstance::Get(contextObj).GetTravelMgr();
   travelMgrForWorld._ServerInitiateTravel(mapName, travelType);
}

/* static */
void UTATTravelMgr::ServerInitiateTravelToWorld(const UObject* contextObj, const TSoftObjectPtr<UWorld>& map, ETATTravelType travelType)
{
   UTATTravelMgr& travelMgrForWorld = UTATGameInstance::Get(contextObj).GetTravelMgr();
   travelMgrForWorld._ServerInitiateTravel(map.GetAssetName(), travelType);
}

void UTATTravelMgr::ShowLoadingScreen()
{
   // loading screens on top to hide the hud setting itself up!
   // Viewport may not exist yet in PIE
   const UWorld* world = GetWorld();
   if (_loadingScreenUserWidget && !_loadingScreenUserWidget->IsInViewport() && world && world->GetGameViewport())
      _loadingScreenUserWidget->AddToViewport(TATUIZOrder::LoadingScreen);
}

void UTATTravelMgr::HideLoadingScreen()
{
   if (_loadingScreenUserWidget)
      _loadingScreenUserWidget->RemoveFromParent();
}

UWorld* UTATTravelMgr::GetWorld() const
{
   if (_gameInstance)
      return _gameInstance->GetWorld();
   return nullptr;
}

void UTATTravelMgr::Tick(float deltaTime)
{
   if (!GetWorld())
      return;
   if (_loadingScreenShowingOnDestinationMap)
   {
      check(_gameInstance);
      if (_gameInstance->IsDedicatedServerInstance())
      {
         // on the dedicated server so get rid of it now?
         _loadingScreenShowingOnDestinationMap = false;
         HideLoadingScreen();
      }
      // Wait for outstanding PSO precompile tasks to finish first
      else if (FShaderPipelineCache::NumPrecompilesRemaining() == 0)
      {
         // wait for the local player to have a controller and character, then we can get rid of the loading screen
         ATATPlayerController* localPC = ATATPlayerController::GetLocalTATPlayerController(this);
         ATATPlayerState* localPS = localPC ? localPC->GetTATPlayerState() : nullptr;
         ATATGameState* gameState = Cast<ATATGameState>(GetWorld()->GetGameState());
         if (localPC && localPS && gameState)
         {
            const UTATMapVariationMgrComponent* mapVariationMgr = gameState->GetMapVariationMgr();
            if (localPC->IsCharacterReady() && (mapVariationMgr == nullptr || mapVariationMgr->IsMapReady()))
            {
               // get rid of it now that we have a character that is ready!
               _loadingScreenShowingOnDestinationMap = false;
               HideLoadingScreen();

               // we're ready for our next intro state
               localPS->LocalSetMapIntroStateComplete(ETATMapIntroState::Loading);

               ATATCharacter* localCharacter = localPC->GetPawn<ATATCharacter>();
               OnLocalPlayerLoadedIntoMap.Broadcast(localPS, localPC, localCharacter);
            }
         }
      }
   }
}

void UTATTravelMgr::_ServerInitiateTravel(const FString& mapName, ETATTravelType travelType)
{
   // cache off the travel state
   _serverDestinationMapName = mapName;

   UWorld* world = GetWorld();
   check(world);

   bool useSeamlessTravel = true;
   switch(travelType)
   {
   case ETATTravelType::WithLoadingScreen:
      {
         useSeamlessTravel = false;
         
         // throw our loading screen up over the viewport in the current scene, which covers us till the movie player pops on top during the load
         ShowLoadingScreen();
      }
      break;
   case ETATTravelType::WithTransitionMap:
      {
         // 2/26/2021: Seamless travel is causing this bug:
         // * Server + a single client seamless travel from hub => prototype map (castle ward)
         // * When we land on the prototype map, the hud tries to build itself for player controller 0
         // Player controller 0 is somehow the client, a remote player, and is relatively broken from that point on
         // So for now we have decided to disable seamless travel -- it was unlikely we were going to get away w/ this implementation anyway as it was
         // quite hitchy, even in package builds, and we're going to have to mask some mission scenario map loads in the future, which a "no loading screen"
         // design didn't really work well for.
         //useSeamlessTravel = true;
         useSeamlessTravel = false;

         // 2/26/2021: ... and show the loading widget just like above
         // throw our loading screen up over the viewport in the current scene, which covers us till the movie player pops on top during the load
         ShowLoadingScreen();
      }
      break;
   }

   // NOTE: Seamless travel is disabled in Play-in-Editor in the current Unreal version due to bugs
   //       This change selectively turns on seamless travel in PIE, so that it can fall back to
   //       non-seamless travel. Please be aware that some behavior may be different in PIE as
   //       a result.
#if WITH_EDITOR
   if (world->IsPlayInEditor())
   {
      useSeamlessTravel = false;
   }
#endif

   // swap to seamless travel on/off
   bool prevSeamlessTravel = true;
   if (AGameModeBase* gameMode = world->GetAuthGameMode())
   {
      prevSeamlessTravel = gameMode->bUseSeamlessTravel;
      gameMode->bUseSeamlessTravel = useSeamlessTravel;
   }

   // updates params in case of viewport geo changes before travel kicks off
   _SetupMoviePlayerForTravel();

   // tell all of our clients we're leaving the map
   for (auto it = world->GetPlayerControllerIterator(); it; ++it)
   {
      ATATPlayerController* playerController = Cast<ATATPlayerController>(it->Get());
      if (playerController && !playerController->IsLocalController())
      {
         playerController->ClientAboutToChangeMaps();
      }
   }

   // server travel
   UE_LOG(LogTATTravelMgr, Log, TEXT("Traveling to %s %s seamless travel"), *_serverDestinationMapName, useSeamlessTravel ? TEXT("with") : TEXT("without"));
   world->ServerTravel(_serverDestinationMapName);

   // fix it back up to the previous value after
   if (AGameModeBase* gameMode = world->GetAuthGameMode())
   {
      gameMode->bUseSeamlessTravel = prevSeamlessTravel;
   }
}

void UTATTravelMgr::_SetupMoviePlayerForTravel()
{
   if (_loadingScreenUserWidget)
   {
      check(_loadingScreenSlateWidget && _loadingScreenSlateWidget == _loadingScreenUserWidget->GetCachedWidget());

      FLoadingScreenAttributes loadScreen;
      loadScreen.WidgetLoadingScreen = _loadingScreenSlateWidget;
      loadScreen.bMoviesAreSkippable = false;

      GetMoviePlayer()->SetupLoadingScreen(loadScreen);

      const UGameViewportClient* gameViewport = _gameInstance->GetGameViewportClient();
      const UUserInterfaceSettings* uiSettings = GetDefault<UUserInterfaceSettings>();

      if (gameViewport && uiSettings)
      {
         FVector2D viewportSize;
         gameViewport->GetViewportSize(viewportSize);

         const float viewportDpiScale = uiSettings->GetDPIScaleBasedOnSize(FIntPoint(viewportSize.X, viewportSize.Y));
         GetMoviePlayer()->SetViewportDPIScale(viewportDpiScale);
      }
   }
}

void UTATTravelMgr::_OnPostLoadMapWithWorld(UWorld* loadedWorld)
{
   if(loadedWorld == nullptr)
   {
      return;
   }
   const ATATWorldSettings& worldSettings = ATATWorldSettings::Get(loadedWorld);
   if (worldSettings.MapType == ETATMapType::Hub ||
       worldSettings.MapType == ETATMapType::ThievesDen ||
       worldSettings.MapType == ETATMapType::Mission ||
       worldSettings.MapType == ETATMapType::ServerStart ||
       worldSettings.MapType == ETATMapType::Developer)
   {
#if OSE_ALLOW_PERFTEST
      // Don't open the loading screen again during a perf test, but crucially, also don't kick
      // off the local-player-ready callbacks so that does not have to be suppressed another way
      if (UOSEPerformanceTestComponent::IsRunningPerformanceTest())
      {
         return;
      }
#endif

      // continue showing our loading screen even after we've landed on our destination map. 
      // we'll wait for our character to replicate down to us, and the HUD will be all setup, before we remove it
      _loadingScreenShowingOnDestinationMap = true;
      ShowLoadingScreen();
   }
}
