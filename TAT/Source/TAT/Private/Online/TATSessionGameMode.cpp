// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Online/TATSessionGameMode.h"

// tat
#include "Character/TATTeams.h"
#include "Coop/TATCooperationGameplayTags.h"
#include "Coop/TATCooperationSettings.h"
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATWorldSettings.h"
#include "Loot/TATLootInventory.h"
#include "Loot/TATLootUtils.h"
#include "Loot/TATStashedLootSubsystem.h"
#include "Matchmaking/TATRankingFunctionLibrary.h"
#include "Matchmaking/TATScoringSettings.h"
#include "Online/TATGameState.h"
#include "Player/TATPlayerController.h"
#include "Player/TATPlayerState.h"
#include "Player/TATPlayerStatsTags.h"
#include "Player/TATCharacter.h"
#include "Quests/TATActiveQuestSubsystem.h"
#include "Quests/TATPlayerObjective.h"
#include "Quests/TATQuestDataSubsystem.h"
#include "Quests/TATQuestInfo.h"
#include "Quests/Rewards/Requirements/TATQuestRewardRequirementType.h"
#include "Progression/TATPlayerExperienceUtils.h"
#include "Online/TATDedicatedServerManagerOnlineSession.h"
#include "TATGameInstance.h"

// ose
#include "Online/OSEGameState.h"
#include "ServerManager/OSEDedicatedServerManagerBase.h"

// ue
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Algo/AnyOf.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSessionGameMode)
DEFINE_LOG_CATEGORY_STATIC(LogTATSessionGameMode, Log, All);

namespace GameModeHelpers
{
   static FMatchPersistentRankingPlayerData CreateRanking(const FMatchPersistentData& persistentData, const APlayerState* playerState)
   {
      check(playerState);
      return FMatchPersistentRankingPlayerData {
         .PlayerName = playerState->GetPlayerName(),
         .CharacterType = persistentData.CharacterType,
         .TotalLootValue = persistentData.GetTotalValue()
      };
   }

   static void SetEscapeTimeStat(AOSEPlayerState* ps)
   {
      const UTATProjectSettings& tatProjectSettings = UTATProjectSettings::Get();
      if (tatProjectSettings.PlayerEscapeTimeRemainingStatTag.IsValid())
      {
         const ATATGameState* gameState = ps->GetWorld()->GetGameState<ATATGameState>();
         check(gameState);
         if (gameState->GetCurrentPhase() == ETATMatchPhase::Endgame)
         {
            const int32 secondsRemaining = static_cast<int32>(gameState->GetTimeLeftInPhase());
            ps->AuthorityUpdatePlayerStatInt(tatProjectSettings.PlayerEscapeTimeRemainingStatTag, secondsRemaining);
         }
      }
   }

   static void AddStashedLoot(FMatchPersistentData& persistentData, uint8 team, const UWorld* world)
   {
      if (UTATStashedLootSubsystem* stashSubsystem = world->GetSubsystem<UTATStashedLootSubsystem>())
      {
         persistentData.StashedLootValue = stashSubsystem->GetStashedValueForTeam(team);
         persistentData.StashedLoot = stashSubsystem->AuthorityGetStashedLootForTeam(team);

      }
   }
}

ATATSessionGameMode::ATATSessionGameMode() : Super()
{
}

void ATATSessionGameMode::PostLogin(APlayerController* newPlayer)
{
   Super::PostLogin(newPlayer);
}

void ATATSessionGameMode::BeginPlay()
{
   Super::BeginPlay();
   _PerformAsyncLoads();

   float maxMatchDurationSeconds = 0.0f;

   // If we get a command line flag, prioritize that value over the config
   {
      float maxMatchDurationMinutes = 0.0f;
      if (FParse::Value(FCommandLine::Get(), TEXT("-MaximumMatchDurationMinutes="), maxMatchDurationMinutes) && maxMatchDurationMinutes > 0)
      {
         maxMatchDurationSeconds = maxMatchDurationMinutes * 60.0f;
      }
   }

   // If we don't get a command line flag and this is a dedicated server (and not the editor), check the config option
   if (maxMatchDurationSeconds <= 0 && (GetNetMode() == NM_DedicatedServer && !GetWorld()->IsEditorWorld()))
   {
      maxMatchDurationSeconds = UTATProjectSettings::Get().MaximumDedicatedServerMatchDurationMinutes * 60.0f;
   }

   // Set a timer to kill the match when the timer is up
   if (maxMatchDurationSeconds > 0.0f)
   {
      static constexpr bool looping = false;
      FTimerHandle handle{};
      GetWorldTimerManager().SetTimer(handle, FTimerDelegate::CreateUObject(this, &ATATSessionGameMode::_OnMatchFailsafeDurationReached), maxMatchDurationSeconds, looping);
   }

   _matchStartTime = GetWorld()->GetTimeSeconds();
}

bool ATATSessionGameMode::HasPlayerEscaped(const APlayerState* ps) const
{
   return _escapedPlayers.Contains(ps);
}

bool ATATSessionGameMode::GetPlayerEscapeOrderPlacement(const APlayerState* ps, int32& outEscapePlacement) const
{
   return _escapedPlayers.Find(ps, outEscapePlacement);
}

void ATATSessionGameMode::PlayerCaught(APlayerController* caughtPlayer)
{
   _HandlePlayersEscapedOrCaught({ caughtPlayer }, nullptr, true);
}

void ATATSessionGameMode::HandlePlayersEscaped(TConstArrayView<APlayerController*> players, ATATEscapePoint* escapePoint)
{
   _HandlePlayersEscapedOrCaught(players, escapePoint, false);
}

void ATATSessionGameMode::HandlePlayersCaught(TConstArrayView<APlayerController*> players)
{
   _HandlePlayersEscapedOrCaught(players, nullptr, true);
}

void ATATSessionGameMode::ForceRemainingPlayersCaught()
{
   auto shouldCountAsEscaped = [](const APlayerState* playerState) -> bool
   {
      if(const UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(playerState, false))
      {
         return asc->HasMatchingGameplayTag(UTATProjectSettings::Get().EligibleForEndOfMatchEscapeTag);
      }
      return false;
   };
   TArray<APlayerController*> remainingCaughtPlayers;
   TArray<APlayerController*> escapingPlayers;
   for(FConstPlayerControllerIterator iterator = GetWorld()->GetPlayerControllerIterator(); iterator; ++iterator)
   {
      if(APlayerController* pc = iterator->Get(false))
      {
         const APlayerState* ps = pc->GetPlayerState<APlayerState>();
         check(ps);

         if (!HasPlayerEscaped(ps) && !_caughtPlayers.Contains(ps))
         {
            if(shouldCountAsEscaped(ps))
            {
               escapingPlayers.Add(pc);
            }
            else
            {
               remainingCaughtPlayers.Add(pc);
            }
         }
      }
   }

   // NB: If only some players in a team escape at the end of a match, they
   //     are treated as if separate as a result of being processed in separate calls.
   HandlePlayersEscaped(escapingPlayers, nullptr);
   HandlePlayersCaught(remainingCaughtPlayers);
}

int ATATSessionGameMode::GetTotalNumberOfActivePlayers() const
{
   int totalActiveNonSpectatorPlayers = 0;
   for (FConstPlayerControllerIterator iterator = GetWorld()->GetPlayerControllerIterator(); iterator; ++iterator)
   {
      const APlayerController* playerActor = iterator->Get();
      if (playerActor && playerActor->PlayerState &&
         playerActor->PlayerState->IsSpectator() == false &&
         playerActor->PlayerState->IsInactive() == false)
      {
         totalActiveNonSpectatorPlayers++;
      }
   }
   return totalActiveNonSpectatorPlayers;
}

void ATATSessionGameMode::HandlePlayerKO(APlayerController* player)
{
   if(ATATPlayerState* tatPs = player->GetPlayerState<ATATPlayerState>())
   {
      tatPs->AuthorityUpdatePlayerStatInt(TAG_PlayerStats_Cooperation_Respawns, 1);

      // CONSIDER: extract some utility to iterate over other player-states with same team
      if (const AOSEGameState* gameState = GetGameState<AOSEGameState>())
      {
         for (AOSEPlayerState* otherPs : gameState->GetOSEPlayerStates())
         {
            if (otherPs != tatPs && otherPs->GetTeam() == tatPs->GetTeam())
            {
               otherPs->AuthorityUpdatePlayerStatInt(TAG_PlayerStats_Cooperation_Respawns, 1);
            }
         }
      }
   }
}

UClass* ATATSessionGameMode::GetDefaultPawnClassForController_Implementation(AController* inController)
{
   ATATPlayerState* playerState = inController->GetPlayerState<ATATPlayerState>();
   if(playerState == nullptr)
      return Super::GetDefaultPawnClassForController_Implementation(inController);

   ETATCharacter character = playerState->GetTATCharacter();
   if(character == ETATCharacter::None)
      return Super::GetDefaultPawnClassForController_Implementation(inController);

   if(const UTATCharactersMetadata* metadata = UTATProjectSettings::Get().DefaultCharacterMetadata.LoadSynchronous())
   {
      const TSoftClassPtr<ATATCharacter> characterBPClass = metadata->GetCharacterMetadata(character).BlueprintClass;
      return characterBPClass.LoadSynchronous();
   }
   
   return Super::GetDefaultPawnClassForController_Implementation(inController);
}

bool ATATSessionGameMode::MustSpectate_Implementation(APlayerController* newPlayerController) const
{
   ATATPlayerState* playerState = newPlayerController->GetPlayerState<ATATPlayerState>();
   if(playerState == nullptr)
      return true;

   return playerState->GetTATCharacter() == ETATCharacter::None;
}

void ATATSessionGameMode::_FinalizeMatchResults()
{
   // send updated rankings
   for (FConstPlayerControllerIterator iterator = GetWorld()->GetPlayerControllerIterator(); iterator; ++iterator)
   {
      ATATPlayerController* otherPlayer = CastChecked<ATATPlayerController>(*iterator);
      otherPlayer->ClientNotifyLeaderboardData(_rankingData);
   }

   if (!_matchEndTimer.IsValid())
   {
      // Give a short delay to the FinishEscaping RPC, since that will cause players to disconnect,
      // and we want to be reasonably sure that they've received any other end-of-match RPCs
      // TODO: Might be better to have an explicit handshake, and poll that all clients either received all their data, or have manually DC'd
      GetWorld()->GetTimerManager().SetTimer(_matchEndTimer,
                                             this,
                                             &ATATSessionGameMode::_OnMatchEnd,
                                             UTATProjectSettings::Get().EndMatchRPCDelaySeconds);
   }
}

void ATATSessionGameMode::_HandlePlayersEscapedOrCaught(TConstArrayView<APlayerController*> controllers, ATATEscapePoint* escapePoint, const bool bCaught)
{
   struct FInfo
   {
      ATATPlayerController* Controller = nullptr;
      ATATPlayerState* PlayerState = nullptr;

      FMatchPersistentData MatchData;
      uint8 Team = 0;

      FInfo(ATATPlayerController* controller, ATATPlayerState* playerState)
      {
         check(controller);
         check(playerState);
         Controller = controller;
         PlayerState = playerState;
         Team = PlayerState->GetTeam();
      }
   };
   TArray<FInfo> playerInfos;
   playerInfos.Reserve(controllers.Num());

   for(APlayerController* controller : controllers)
   {
      check(controller);
      ATATPlayerState* playerState = controller->GetPlayerState<ATATPlayerState>();
      ATATPlayerController* tatController = Cast<ATATPlayerController>(controller);
      
      if (playerState && tatController)
      {
         playerInfos.Emplace(tatController, playerState);
      }
   }

   for(FInfo& info : playerInfos)
   {
      if (!bCaught)
      {
         const bool alreadyEscaped = _escapedPlayers.Contains(info.PlayerState);
         _escapedPlayers.AddUnique(info.PlayerState);
         UE_CLOG(alreadyEscaped, LogTATSessionGameMode, Warning, TEXT("_HandlePlayerEscapedOrCaught() | escaping player %s already in _escapedPlayers!"), *info.PlayerState->GetPlayerName());
         GameModeHelpers::SetEscapeTimeStat(info.PlayerState);
      }
      else
      {
         _caughtPlayers.AddUnique(info.PlayerState);
      }
   }

   for(FInfo& info : playerInfos)
   {
      _CalculateInitialMatchPersistentData(info.PlayerState, info.MatchData);
   }
   

   // Share team stuff
   // ASSUMPTION: Players are forced to escape together. If some players may exit (or die) with state that we need to share,
   //             will need to hanlde that
   for (int i = 0; i < playerInfos.Num(); ++i)
   {
      FInfo& info = playerInfos[i];
      for (int otherPlayerIndex = 0; otherPlayerIndex < playerInfos.Num(); ++otherPlayerIndex)
      {
         const FInfo& otherInfo = playerInfos[otherPlayerIndex];
         if (otherPlayerIndex == i || otherInfo.Team != info.Team)
         {
            continue;
         }

         // Add to TeamLoot, assuming that it should count for all players
         // CONSIDER: How should contract loot be treated?
         info.MatchData.AllyCarriedLoot.Append(otherInfo.MatchData.CarriedLoot);
         info.MatchData.AllyCarriedLootValue += otherInfo.MatchData.KeptCarriedLootValue;
         info.MatchData.HasCoopAllies = true;
      }
   }

   for(FInfo& info : playerInfos)
   {
      _CalculateQuestResult(info.MatchData, info.PlayerState, !bCaught);

      // Update player stats with the final number of major/minor
      // Even if we later update these player stats during match play, we still want to use the loot in results as ground truth (as quests do)
      info.MatchData.PlayerStats.GetOrAddStat(TAG_PlayerStats_Loot_Extracted_Major).IntValue = info.MatchData.CountAllLootOfType(this, ETATLootType::MajorLoot);
      info.MatchData.PlayerStats.GetOrAddStat(TAG_PlayerStats_Loot_Extracted_Minor).IntValue = info.MatchData.CountAllLootOfType(this, ETATLootType::MinorLoot);

      info.MatchData.XPGained = UTATPlayerExperienceUtils::CalculateFinishedMatchXP(this, info.MatchData, info.PlayerState->AuthorityGetAdditionalXPWhenMatchEnds(), info.MatchData.XPGainedArray);

      // TODO: is this still relevant
      info.MatchData.MatchRanking = UTATRankingFunctionLibrary::GenerateMatchRankingForPlayer(info.MatchData);
   }

   for(const FInfo& info : playerInfos)
   {
      FMatchPersistentRankingPlayerData rankData = GameModeHelpers::CreateRanking(info.MatchData, info.PlayerState);
      _rankingData.Add(rankData);
   }

   for (const FInfo& info : playerInfos)
   {
      info.PlayerState->AuthoritySetMatchCompletionState(bCaught ? EMatchCompletionState::Captured : EMatchCompletionState::Escaped);
   }

   for(const FInfo& info : playerInfos)
   {
      // Notify all players of the escape, and tell the escaping player that they should start spectating
      _CreateAndSendClientEscapedRPC(info.Controller, info.MatchData, escapePoint);
   }

   // Notify if any escaped with mission goal
   if (!bCaught && Algo::AnyOf(playerInfos, [](const FInfo& info) { return info.MatchData.MissionResult.IsObjectiveComplete; }))
   {
      if (auto* gameState = GetGameState<ATATGameState>())
      {
         gameState->AuthorityOnPlayerEscapedWithMatchObjective();
      }
   }

   // Check if every player has escaped or been captured: if so, end the match
   if (GetTotalNumberOfActivePlayers() == 0)
   {
      _FinalizeMatchResults();
   }
}

void ATATSessionGameMode::_UpdateOtherPlayersOfEscape(const FMatchPersistentData& persistentData,
                                                  const ATATPlayerController* controller,
                                                  ATATEscapePoint* escapePoint) const
{
   const FString escapingPlayerName = controller->GetPlayerState<APlayerState>()->GetPlayerName();
   for(FConstPlayerControllerIterator iterator = GetWorld()->GetPlayerControllerIterator(); iterator; ++iterator)
   {
      ATATPlayerController* otherPlayer = Cast<ATATPlayerController>(*iterator);
      if(otherPlayer && otherPlayer != controller)
      {
         otherPlayer->ClientNotifyAnotherPlayerEscaped(escapingPlayerName, escapePoint, persistentData.GetTotalValue());
      }
   }
}

void ATATSessionGameMode::_CreateAndSendClientEscapedRPC(
   ATATPlayerController* controller,
   const FMatchPersistentData& persistentData,
   ATATEscapePoint* escapePoint)
{
   const ATATPlayerState* tatPS = controller->GetTATPlayerState();
   check(tatPS);

   // Enter spectator mode after being told to escape
   if(tatPS->IsSpectator() == false)
   {
      if(APawn* currentPawn = controller->GetPawn())
      {
         // We can't instantly destroy the pawn here.
         // If we did the `PawnPendingDestroy` function on the clients will trigger before they enter the spectating
         // state, which in turn puts them into the inactive state, stomping the spectator state.
         // -- side note, the pawn should probably play an "escaping" animation anyway, maybe?
         currentPawn->SetLifeSpan(1.f);
         controller->StartSpectatingOnly();
         currentPawn->UnPossessed();
      }
   }
   
   // Stash the match data, this will also set the client into a spectating mode
   controller->ClientStashMatchData(persistentData);
 
   if(persistentData.CompletionState == EMatchCompletionState::Escaped)
   {
      _UpdateOtherPlayersOfEscape(persistentData, controller, escapePoint);
   }
}

void ATATSessionGameMode::_CalculateQuestResult(FMatchPersistentData& persistentData, ATATPlayerState* tatPS, const bool didEscape) const
{
   const FTATQuestRewardRequirementContext questRewardRequirementContext
   {
   };

   const UTATQuestDataSubsystem* questDataSubsystem = &UTATQuestDataSubsystem::Get(this);
   auto populateQuestResult = [tatPS, didEscape, &persistentData, questRewardRequirementContext, questDataSubsystem](ETATPlayerQuestSlot questSlot, FMatchPersistentQuestResult& result)
   {
      if(const UTATRootPlayerObjective* objective = tatPS->GetObjectiveForSlot(questSlot))
      {
         result.QuestTag = objective->QuestTag;
         result.IsObjectiveComplete = objective->IsCompleteForMatchEnd(didEscape, persistentData);

         // ASSUMPTION: only grant rewards on quest completion (this may change with reward levels/requirements)
         if (result.IsObjectiveComplete)
         {
            check(questDataSubsystem != nullptr);
            if (const FTATMissionInfo* missionInfo = questDataSubsystem->FindMissionInfo(result.QuestTag))
            {
               result.UnlockedBonusRewardsBitmask = missionInfo->GetUnlockedBonusRewardsBitmask(questRewardRequirementContext);
            }
         }
      }
   };
   populateQuestResult(ETATPlayerQuestSlot::Mission, persistentData.MissionResult);
   populateQuestResult(ETATPlayerQuestSlot::Contract, persistentData.ContractResult);

   if (persistentData.ContractResult.QuestTag.IsValid())
   {
      if (const UTATActiveQuestSubsystem* activeQuestSubsystem = GetWorld()->GetSubsystem<UTATActiveQuestSubsystem>())
      {
         persistentData.ContractResult.IsAccomplice = activeQuestSubsystem->IsPlayerAccompliceForContract(tatPS->GetPlayerId());
      }
   }
}

void ATATSessionGameMode::_CalculateInitialMatchPersistentData(ATATPlayerState* tatPS, FMatchPersistentData& persistentData) const
{
   check(tatPS);

   int32 escapeOrderPlacement = -1;
   const bool didEscape = GetPlayerEscapeOrderPlacement(tatPS, escapeOrderPlacement);

   const UTATLootInventoryComponent* escapingPlayerLoot = tatPS->GetLootInventoryComponent();

   // Determine data for the player's results screen
   escapingPlayerLoot->AuthorityStashDataForEscape(persistentData, !didEscape);
   GameModeHelpers::AddStashedLoot(persistentData, tatPS->GetTeam(), GetWorld());
   persistentData.CharacterType = tatPS->GetTATCharacter();
   persistentData.CharacterSaveId = tatPS->GetCharacterSaveId();
   persistentData.CompletionState = didEscape ? EMatchCompletionState::Escaped : EMatchCompletionState::Captured;
   persistentData.EscapeOrderPlacement = escapeOrderPlacement;
   persistentData.PlayerStats = tatPS->GetPlayerStats();
   persistentData.MatchTimeInSeconds = GetWorld()->GetTimeSeconds() - _matchStartTime;
   persistentData.MatchDifficulty = TATDifficulty::GetDifficultyForMatch(GetWorld());
   const ATATWorldSettings* worldSettings = CastChecked<ATATWorldSettings>(GetWorld()->GetWorldSettings());
   persistentData.IsFTUEMatch = worldSettings->MapType == ETATMapType::Tutorial;

   const FTATCachedPlayerInfo& playerKnockedOutByInfo = tatPS->GetPlayerKnockedOutByInfo();
   persistentData.WasKilledByPlayer = playerKnockedOutByInfo.IsValid();
   persistentData.PlayerKnockedOutByInfo = playerKnockedOutByInfo;

   if (const UTATGameInstance* gameInstance = GetGameInstance<UTATGameInstance>())
   {
      persistentData.WasMatchmade = gameInstance->WasPartyMatchmade();
   }
}

bool ATATSessionGameMode::_HasActivePlayers() const
{
   for (FConstPlayerControllerIterator iterator = GetWorld()->GetPlayerControllerIterator(); iterator; ++iterator)
   {
      const APlayerController* playerActor = iterator->Get();
      if (playerActor &&
         playerActor->PlayerState &&
         playerActor->PlayerState->IsSpectator() == false &&
         playerActor->PlayerState->IsInactive() == false &&
         !_caughtPlayers.Contains(playerActor->PlayerState) &&
         !_escapedPlayers.Contains(playerActor->PlayerState))
      {
         return true;
      }
   }
   return false;
}

void ATATSessionGameMode::_PerformAsyncLoads()
{
   FStreamableManager& streamableManager = UAssetManager::GetStreamableManager();

   // Kick the async-load of matchmaking settings soft-referenced bonus data, so it's ready by the time a player escapes / is KO'd
   const UTATScoringSettings& scoringSettings = UTATScoringSettings::Get();
   _escapeOrderBonusHandle = streamableManager.RequestAsyncLoad(scoringSettings.EscapeOrderBonus.ToSoftObjectPath());
}

void ATATSessionGameMode::_OnMatchFailsafeDurationReached()
{
   UE_LOG(LogTATSessionGameMode, Error, TEXT("Match failsafe duration reached"));

   // If we haven't run OnMatchEnd yet, just call that and let it schedule the cleanup a few seconds later
   if (!_ranOnMatchEnd)
   {
      _OnMatchEnd();
   }
}

void ATATSessionGameMode::_OnMatchEnd()
{
   // Send an RPC to notify player controllers that the match has ended
   for (FConstPlayerControllerIterator iterator = GetWorld()->GetPlayerControllerIterator(); iterator; ++iterator)
   {
      ATATPlayerController* otherPlayer = CastChecked<ATATPlayerController>(*iterator);
      otherPlayer->ClientFinishEscaping();
   }

   //TODO: This would be a good place for hooks into a future progression/orchestration system to indicate that this server is done and about to shut down
   _ranOnMatchEnd = true;
   
   if(UOSEDedicatedServerManagerBase* serverMgr = UOSEDedicatedServerManagerBase::TryGet(*this))
   {
      serverMgr->ServerEndMatch();
   }
}
