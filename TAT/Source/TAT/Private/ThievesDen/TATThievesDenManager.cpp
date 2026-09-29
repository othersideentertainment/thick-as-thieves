// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "ThievesDen/TATThievesDenManager.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATTravelMgr.h"
#include "TATGameInstance.h"
#include "ThievesDen/TATThievesDenGameState.h"
#include "ThievesDen/TATThievesDenPlayerController.h"
#include "UI/Queue/TATUIQueue.h"
#include "UI/Quests/TATQuestFlowUtils.h"
#include "UI/TATScreenWidget.h"
#include "UI/TATScreenMgr.h"
#include "GameFramework/TATMatchPersistenceGameInstanceSubsystem.h"
#include "SaveGame/TATSaveGame.h"
#include "UI/XP/TATEndMatchXPScreen.h"
#include "Unlockables/TATUnlockableContent.h"

// ue
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATThievesDenManager)


namespace ThievesDenHelpers
{
   static void AddUnlockTutorials(const FMatchPersistentXPGainedData& xpGainData, const FMatchPersistentData& matchData, UWorld* world)
   {
      // NOTE: This is a slightly hold-your-nose glue to kick off a tutorial after a match that unlocked a thing
      //       Not the best long-term structure
      UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(world);
      check(saveGame);
      auto isUnlockedByMatch = [&](const FTATUnlockCondition& condition)
         {
            switch (condition.UnlockType)
            {
            case ETATUnlockType::RequiredLevel:
               return xpGainData.LevelBeforeXPGain < condition.RequiredLevel && saveGame->GetXP().Level >= condition.RequiredLevel;
            case ETATUnlockType::RequiredContract:
               return matchData.ContractResult.IsCompleteForMetagame() && matchData.ContractResult.QuestTag == condition.RequiredContractTag;
            default:
               return false;
            }
         };

      const UTATProjectSettings& settings = UTATProjectSettings::Get();
      UTATUnlockableContentDataAsset* unlockData = settings.UnlockableContentData.LoadSynchronous();
      if (!ensure(unlockData))
      {
         return;
      }

      for (const FTATUnlockableContent& contentEntry : unlockData->EntryTable)
      {
         if (!settings.PostMatchUnlockTutorials.Contains(contentEntry.Tag))
         {
            continue;
         }

         if (contentEntry.UnlockRequirement.UnlockConditions.ContainsByPredicate(isUnlockedByMatch))
         {
            UClass* tutorialRunnerClass = settings.PostMatchUnlockTutorials[contentEntry.Tag].LoadSynchronous();
            if (ensure(tutorialRunnerClass))
            {
               world->SpawnActor(tutorialRunnerClass);
            }
            return;
         }
      }
   }
}

ATATThievesDenManager::ATATThievesDenManager()
{
   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = true;
   bReplicates = true;
   bAlwaysRelevant = true;
}

// static
ATATThievesDenManager* ATATThievesDenManager::Get(const UObject* worldContextObject)
{
   // ATATThievesDenGameState always has a pointer to the thieves den manager
   if (UWorld* world = GEngine->GetWorldFromContextObject(worldContextObject, EGetWorldErrorMode::ReturnNull))
   {
      if (ATATThievesDenGameState* gameState = world->GetGameState<ATATThievesDenGameState>())
      {
         return gameState->GetThievesDenManager();
      }
   }
   return nullptr;
}

void ATATThievesDenManager::BeginPlay()
{
   Super::BeginPlay();

   if (PostMatchScreenWidget)
   {
      GetWorldTimerManager().SetTimerForNextTick([weakThis = MakeWeakObjectPtr(this)]()
      {
         if (ATATThievesDenManager* self = weakThis.Get())
         {
            self->_ShowPostMatchScreenIfNeeded();
         }
      });
   }
}

void ATATThievesDenManager::Tick(float deltaSeconds)
{
   Super::Tick(deltaSeconds);

   // on the server, wait for all players to be inside of the marker
   if (HasAuthority())
   {
      const ATATGameState* gameState = GetWorld()->GetGameState<ATATGameState>();
      const float serverWorldTimeNow = GetWorld()->GetTimeSeconds();
      if (!ensure(gameState != nullptr))
      {
         return;
      }

      // all players are ready to go, start the countdown
      float newServerTravelWorldTime = _authorityTravelWorldTime;
      if (gameState->GetAreAllPlayersReadyChecked())
      {
         if (_authorityTravelWorldTime == static_cast<float>(INDEX_NONE))
         {
            newServerTravelWorldTime = serverWorldTimeNow + SecondsToTravel;
         }
      }
      else
      {
         newServerTravelWorldTime = static_cast<float>(INDEX_NONE);
      }

      if (newServerTravelWorldTime != _authorityTravelWorldTime)
      {
         _authorityTravelWorldTime = newServerTravelWorldTime;
         _UpdateHUDMissionCountdown();
      }

      // travel!
      if (_authorityTravelWorldTime != static_cast<float>(INDEX_NONE) && serverWorldTimeNow > _authorityTravelWorldTime && !_authorityTravelStarted && !_authoritySelectedMap.IsNull())
      {
         _AuthorityUpdateParty();
         _authorityTravelStarted = true;
         _OnAuthorityTravelToMap(_authoritySelectedMap);
      }
   }
}

void ATATThievesDenManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
   DOREPLIFETIME(ATATThievesDenManager, _authorityTravelWorldTime);
   DOREPLIFETIME(ATATThievesDenManager, _authoritySelectedMap);
}

void ATATThievesDenManager::AuthoritySetSelectedMap(const TSoftObjectPtr<UWorld>& newMap)
{
   check(HasAuthority());
   _authoritySelectedMap = newMap;
   OnSelectedMapChanged.Broadcast();
}

void ATATThievesDenManager::_OnAuthorityTravelToMap(const TSoftObjectPtr<UWorld>& map)
{
   UTATTravelMgr::ServerInitiateTravelToWorld(this, map, ETATTravelType::WithLoadingScreen);
}

void ATATThievesDenManager::_ShowPostMatchScreenIfNeeded()
{
   UTATMatchPersistenceGameInstanceSubsystem* matchDataSubsystem = UTATGameInstance::Get(this).GetSubsystem<UTATMatchPersistenceGameInstanceSubsystem>();
   if (matchDataSubsystem == nullptr || !matchDataSubsystem->HasDataToPresent())
   {
      return;
   }

   ATATPlayerController* localController = nullptr;
   for (FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator(); it; ++it)
   {
      ATATPlayerController* pc = Cast<ATATPlayerController>(*it);
      if (pc != nullptr && pc->IsLocalPlayerController())
      {
         localController = pc;
         break;
      }
   }

   if (localController == nullptr)
   {
      return;
   }

   const FMatchPersistentData& matchData = matchDataSubsystem->GetMatchPersistentData();

   UTATUIQueue* queue = UTATUIQueue::Create(localController);
   queue->AddAction(UTATScreenQueueAction::Create(*PostMatchScreenWidget));
   
   queue->AddAction(UTATEndMatchXPScreen::CreateAction(matchDataSubsystem->GetPersistentXPGainedData()));

   // Probably not as relevant for missions anymore (but can re-add)
   // UTATQuestFlowUtils::AddMissionCompleteFlow(queue, matchData.MissionResult.QuestTag, matchData.MissionResult.IsObjectiveComplete);
   UTATQuestFlowUtils::AddPostMatchContractFlow(queue, matchData.ContractResult);

   queue->Run();

   ThievesDenHelpers::AddUnlockTutorials(matchDataSubsystem->GetPersistentXPGainedData(), matchData, GetWorld());
}

void ATATThievesDenManager::_OnRep_AuthoritySelectedMap()
{
   OnSelectedMapChanged.Broadcast();
}

void ATATThievesDenManager::_OnRep_ServerTravelWorldTime()
{
   _UpdateHUDMissionCountdown();
}

void ATATThievesDenManager::_UpdateHUDMissionCountdown()
{
   OnTravelWorldTimeChanged.Broadcast(_authorityTravelWorldTime);
}

void ATATThievesDenManager::_AuthorityUpdateParty()
{
   check(HasAuthority());
   const AGameStateBase* gameState = GetWorld()->GetGameState();
   check(gameState != nullptr);

   UTATGameInstance* gameInstance = GetWorld()->GetGameInstance<UTATGameInstance>();
   check(gameInstance != nullptr);

   // Tell the game instance this is our new player group (party)
   gameInstance->ClearParty();
   for (const TObjectPtr<APlayerState>& playerState : gameState->PlayerArray)
   {
      if (playerState != nullptr)
      {
         gameInstance->AddPlayerToParty(playerState->GetUniqueId());
      }
   }
}
