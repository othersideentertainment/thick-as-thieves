// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "ThievesDen/TATThievesDenPlayerController.h"

// tat
#include "ThievesDen/TATThievesDenGameState.h"
#include "UI/TATScreenMgr.h"
#include "UI/TATScreenWidget.h"
#include "ThievesDen/TATThievesDenManager.h"
#include "TATGameInstance.h"
#include "Settings/TATMatchSettingsBase.h"
#include "SaveGame/Proxy/TATSavedLootInventoryUIProxyComponent.h"

// ue
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATThievesDenPlayerController)
DEFINE_LOG_CATEGORY_STATIC(LogTATThievesDenPlayerController, Log, All);

ATATThievesDenPlayerController::ATATThievesDenPlayerController(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   SavedLootInventoryUIProxy = CreateDefaultSubobject<UTATSavedLootInventoryUIProxyComponent>(TEXT("SavedLootInventoryUIProxyComponent"));
}

void ATATThievesDenPlayerController::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (UWorld* world = GetWorld())
   {
      world->GameStateSetEvent.RemoveAll(this);
      if (ATATThievesDenGameState* gameState = world->GetGameState<ATATThievesDenGameState>())
      {
         gameState->OnThievesDenScreenChanged.RemoveAll(this);
      }
   }

   Super::EndPlay(endPlayReason);
}

void ATATThievesDenPlayerController::ClientSetHUD_Implementation(TSubclassOf<AHUD> newHUDClass)
{
   Super::ClientSetHUD_Implementation(newHUDClass);

   UWorld* world = GetWorld();
   check(world != nullptr);

   if (AGameStateBase* gs = world->GetGameState())
   {
      _OnGameStateSet(gs);
   }
   else
   {
      world->GameStateSetEvent.AddUObject(this, &ATATThievesDenPlayerController::_OnGameStateSet);
   }
}

void ATATThievesDenPlayerController::SetDestinationMap(const TSoftObjectPtr<UWorld>& newMap)
{
   _ServerSetDestinationMap(newMap);
}

void ATATThievesDenPlayerController::ServerRequestSetThievesDenScreen_Implementation(ETATThievesDenScreen newScreen)
{
   check(HasAuthority());
   ATATThievesDenGameState* gameState = GetWorld()->GetGameState<ATATThievesDenGameState>();

   // Only allow the mission owner to set the screen in the thieves den
   if (gameState != nullptr && gameState->IsMissionOwner(GetPlayerState<APlayerState>()))
   {
      gameState->AuthoritySetThievesDenScreen(newScreen);
   }
}

void ATATThievesDenPlayerController::_OnGameStateSet(AGameStateBase* gameState)
{
   // Bind to thieves den screen state changes
   ATATThievesDenGameState* thievesDenGameState = CastChecked<ATATThievesDenGameState>(gameState);
   thievesDenGameState->OnThievesDenScreenChanged.AddUObject(this, &ATATThievesDenPlayerController::_OnThievesDenScreenChanged);

   // Unbind from world's GameStateSetEvent
   GetWorld()->GameStateSetEvent.RemoveAll(this);

   // Setup from current match lobby state (if assigned)
   const ETATThievesDenScreen currentScreen = thievesDenGameState->GetThievesDenScreen();
   if (currentScreen != ETATThievesDenScreen::None)
   {
      _OnThievesDenScreenChanged(ETATThievesDenScreen::None, currentScreen);
   }
}

void ATATThievesDenPlayerController::_OnThievesDenScreenChanged(ETATThievesDenScreen oldScreen, ETATThievesDenScreen newScreen)
{
   UE_LOG(LogTATThievesDenPlayerController, Verbose, TEXT("Thieves den switching from %s to %s"),
      *UEnum::GetValueAsString(oldScreen), *UEnum::GetValueAsString(newScreen));
   if (IsNetMode(NM_DedicatedServer))
   {
      return;
   }

   check(IsLocalPlayerController());

   // The game state event OnThievesDenScreenChanged shouldn't ever be fired if oldScreen == newScreen
   ensure(oldScreen != newScreen);

   UTATScreenMgr* screenMgr = GetTATScreenMgr();
   check(screenMgr != nullptr);

   // Remove screen pertaining to last state, if present
   if (oldScreen != ETATThievesDenScreen::None)
   {
      TSubclassOf<UTATScreenWidget> oldScreenClass = _GetScreenForState(oldScreen);
      UTATScreenWidget* topScreen = screenMgr->GetTopScreen();
      if (topScreen && topScreen->IsA(oldScreenClass))
      {
         UE_LOG(LogTATThievesDenPlayerController, Verbose, TEXT("Removing screen %s from previous state (%s) "),
            *topScreen->GetName(), *UEnum::GetValueAsString(oldScreen));
         screenMgr->RemoveScreen(topScreen);
      }
   }

   // Add new screen
   if (newScreen != ETATThievesDenScreen::None)
   {
      const TSubclassOf<UTATScreenWidget> screenClass = _GetScreenForState(newScreen);
      if (IsValid(screenClass))
      {
         UE_LOG(LogTATThievesDenPlayerController, Verbose, TEXT("Creating screen for new state (%s) "), *UEnum::GetValueAsString(newScreen));
         UTATScreenWidget* screenWidget = CreateWidget<UTATScreenWidget>(this, screenClass);
         check(IsValid(screenWidget));
         screenMgr->AddScreen(screenWidget,
            [weakThis = MakeWeakObjectPtr(this), newScreen](UTATScreenWidget* widget)
            {
               if (ATATThievesDenPlayerController* self = weakThis.Get())
               {
                  check(self->IsLocalPlayerController());

                  // Clear the thieves den screen on the game state
                  self->ServerRequestSetThievesDenScreen(ETATThievesDenScreen::None);

                  // Broadcast event
                  self->OnThievesDenScreenWidgetRemoved.Broadcast(newScreen, widget);
               }
            });
         OnThievesDenScreenWidgetAdded.Broadcast(newScreen, screenWidget);
      }
      else
      {
         UE_LOG(LogTATThievesDenPlayerController, Error, TEXT("_GetScreenForState() returned invalid screen class for state %s!"),
            *UEnum::GetValueAsString(newScreen));
      }
   }
}

TSubclassOf<UTATScreenWidget> ATATThievesDenPlayerController::_GetScreenForState(ETATThievesDenScreen matchScreen) const
{
   switch (matchScreen)
   {
   case ETATThievesDenScreen::SetupMatch:
      return SetupMatchScreenWidget;

   case ETATThievesDenScreen::JoinMatch:
      return nullptr;

   default:
      checkNoEntry();
   }
   return TSubclassOf<UTATScreenWidget>();
}

void ATATThievesDenPlayerController::_ServerSetDestinationMap_Implementation(const TSoftObjectPtr<UWorld>& newMap)
{
   if (ATATThievesDenManager* mgr = ATATThievesDenManager::Get(this))
   {
      mgr->AuthoritySetSelectedMap(newMap);
   }
}

void ATATThievesDenPlayerController::_ClientMatchSettingsUpdated_Implementation(const TArray<uint8>& serializedMatchSettings)
{
   if (!ensure(serializedMatchSettings.Num() > 0))
   {
      return;
   }
   UTATMatchSettingsBase& matchSettings = UTATGameInstance::Get(this).GetMatchSettings();
   matchSettings.DeserializeFromByteArray(serializedMatchSettings);
}
