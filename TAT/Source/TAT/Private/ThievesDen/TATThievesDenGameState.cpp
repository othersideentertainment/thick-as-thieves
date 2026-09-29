// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "ThievesDen/TATThievesDenGameState.h"

// tat
#include "ThievesDen/TATThievesDenPlayerController.h"
#include "Player/TATPlayerState.h"
#include "ThievesDen/TATThievesDenManager.h"

// ue
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATThievesDenGameState)
DEFINE_LOG_CATEGORY_STATIC(LogTATThievesDenGameState, Log, All);

ATATThievesDenGameState::ATATThievesDenGameState(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer
      .DoNotCreateDefaultSubobject(TEXT("MapVariationMgr"))) // we don't need MapVariationMgr in the hub, that's a game-side construct.  TODO: Make a gameplay-specific gamestate, too?
{
}

// static
ATATThievesDenGameState* ATATThievesDenGameState::Get(const UObject* worldContext)
{
   if (UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::ReturnNull))
   {
      return world->GetGameState<ATATThievesDenGameState>();
   }
   return nullptr;
}

void ATATThievesDenGameState::PostInitProperties()
{
   Super::PostInitProperties();
   if (HasAnyFlags(RF_ClassDefaultObject) || GetWorld() == nullptr)
   {
      return;
   }

   // Find the thieves den manager actor if we have one.
   // This way other thieves den related logic can trivially get to this actor via the game state instead of needing to search for it.
   for (TActorIterator<ATATThievesDenManager> it = TActorIterator<ATATThievesDenManager>(GetWorld()); it; ++it)
   {
      if (ATATThievesDenManager* mgr = *it)
      {
         _thievesDenManager = mgr;
         break;
      }
   }
}

void ATATThievesDenGameState::BeginPlay()
{
   Super::BeginPlay();
}

void ATATThievesDenGameState::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   Super::EndPlay(endPlayReason);
}

void ATATThievesDenGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATThievesDenGameState, _thievesDenScreen);
}

void ATATThievesDenGameState::AuthoritySetThievesDenScreen(ETATThievesDenScreen newThievesDenScreen)
{
   check(HasAuthority());
   if (_thievesDenScreen == newThievesDenScreen)
   {
      return;
   }

   const ETATThievesDenScreen oldThievesDenScreen = _thievesDenScreen;
   UE_LOG(LogTATThievesDenGameState, Verbose, TEXT("Setting thieves den screen from %s -> %s"),
      *UEnum::GetValueAsString(_thievesDenScreen), *UEnum::GetValueAsString(newThievesDenScreen));

   _thievesDenScreen = newThievesDenScreen;
   OnThievesDenScreenChanged.Broadcast(oldThievesDenScreen, _thievesDenScreen);
}

void ATATThievesDenGameState::_OnRep_ThievesDenScreen(ETATThievesDenScreen oldState)
{
   OnThievesDenScreenChanged.Broadcast(oldState, _thievesDenScreen);
}
