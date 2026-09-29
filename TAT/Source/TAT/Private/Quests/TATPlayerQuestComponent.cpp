// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/TATPlayerQuestComponent.h"

// tat
#include "Net/TATIrisGroupSubsystem.h"
#include "Online/TATGameState.h"
#include "Player/TATLocalPlayerStateWorldSubsystem.h"
#include "Quests/TATActiveQuestSubsystem.h"
#include "Quests/TATPlayerObjective.h"
#include "Quests/TATQuestDataSubsystem.h"
#include "Quests/TATQuestObjective.h"
#include "Quests/TATQuestTags.h"
#include "Quests/TATSharedObjectiveSubsystem.h"

// ose
#include "Character/OSETeamInterface.h"

// ue
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPlayerQuestComponent)


// Sets default values for this component's properties
UTATPlayerQuestComponent::UTATPlayerQuestComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
   SetIsReplicatedByDefault(true);
}


// Called when the game starts
void UTATPlayerQuestComponent::BeginPlay()
{
   Super::BeginPlay();

   // Speculative fix, if it is possible for on-rep to fire before local-ness of player state (which depends on controller) is known
   if (_objectives.Num() > 0)
   {
      _OnActiveQuestsSet();
   }
}

void UTATPlayerQuestComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (GetOwner() && GetOwner()->HasAuthority())
   {
      if (UTATSharedObjectiveSubsystem* sharedObjectiveSubsystem = GetWorld()->GetSubsystem<UTATSharedObjectiveSubsystem>())
      {
         if (const APlayerState* playerState = GetOwner<APlayerState>())
         {
            sharedObjectiveSubsystem->UnregisterPlayer(playerState->GetPlayerId());
         }
      }
   }

   Super::EndPlay(endPlayReason);
}

void UTATPlayerQuestComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;
   params.Condition = COND_OwnerOnly;
   DOREPLIFETIME_WITH_PARAMS_FAST(UTATPlayerQuestComponent, _objectives, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(UTATPlayerQuestComponent, _relatedLootIds, params);
}

void UTATPlayerQuestComponent::_TryStartObjectiveTracker(ETATPlayerQuestSlot slot, FGameplayTag questTag, const FTATQuestObjectiveInfo* objectiveInfo)
{
   if (!questTag.IsValid())
   {
      return;
   }

   if (!objectiveInfo)
   {
      return;
   }

   APlayerState* playerState = GetOwner<APlayerState>();
   UTATRootPlayerObjective* playerObjective = NewObject<UTATRootPlayerObjective>(this);
   playerObjective->AuthorityInitRoot(slot, *objectiveInfo, FTATPlayerObjectiveInitContext {
      .PlayerState = playerState,
      .QuestTag = questTag,
   });
   _objectives.Add(playerObjective);
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _objectives, this);
   AddReplicatedSubObject(playerObjective, COND_OwnerOnly);
   for (UTATPlayerObjective* child : playerObjective->GetChildObjectives())
   {
      AddReplicatedSubObject(child, COND_OwnerOnly);
   }

   if (UTATSharedObjectiveSubsystem* sharedObjectiveSubsystem = GetWorld()->GetSubsystem<UTATSharedObjectiveSubsystem>())
   {
      const int32 playerId = playerState->GetPlayerId();

      auto isCompatibleForSharing = [] (const UTATPlayerObjective* o) { return o->TargetProgress == 0; };

      if (playerObjective->GetChildObjectives().IsEmpty() && isCompatibleForSharing(playerObjective))
      {
         sharedObjectiveSubsystem->RegisterObjective(playerId, playerObjective);
      }
      
      for (UTATPlayerObjective* child : playerObjective->GetChildObjectives())
      {
         if (isCompatibleForSharing(child))
         {
            sharedObjectiveSubsystem->RegisterObjective(playerId, child);
         }
      }
   }

   objectiveInfo->ForEachRelatedLootTag([this](const FGameplayTag& lootIdTag)
   {
      _relatedLootIds.AddUnique(FTATLootIdentifier(lootIdTag));
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _relatedLootIds, this);
   });
}

void UTATPlayerQuestComponent::AuthorityInitializeQuest(int32 playerId)
{
   check(GetOwner()->HasAuthority());

   if (UTATSharedObjectiveSubsystem* sharedObjectiveSubsystem = GetWorld()->GetSubsystem<UTATSharedObjectiveSubsystem>())
   {
      // Ad-hoc coop may not come back, so just set team once for now
      sharedObjectiveSubsystem->SetPlayerTeam(playerId, CastChecked<IOSETeamInterface>(GetOwner())->GetTeam());
   }

   // done as a separate step because PlayerId isn't initialize yet in BeginPlay
   if (const UTATActiveQuestSubsystem* questSubsystem = GetWorld()->GetSubsystem<UTATActiveQuestSubsystem>())
   {
      _TryStartObjectiveTracker(ETATPlayerQuestSlot::Mission, questSubsystem->GetMission(), questSubsystem->GetMissionObjective());
      _TryStartObjectiveTracker(ETATPlayerQuestSlot::Contract, questSubsystem->GetContract(), questSubsystem->GetContractObjective());
      
      _OnActiveQuestsSet();
      _OnRep_RelatedLootIds();
      AuthorityAddConnectionToContractGroups();
   }
}

void UTATPlayerQuestComponent::AuthorityAddConnectionToContractGroups()
{
   check(GetOwner()->HasAuthority());
   if(UTATIrisGroupSubsystem* groupSubsystem = GetWorld()->GetSubsystem<UTATIrisGroupSubsystem>())
   {
      for(const FGameplayTag& questTag : GetActiveQuestTags())
      {
         if(questTag.MatchesTag(TAG_Contract))
         {
            groupSubsystem->ForTag(questTag).AllowGroupForPlayer(GetOwner());
         }
      }
   }
}

void UTATPlayerQuestComponent::AuthorityForceObjectiveComplete(ETATPlayerQuestSlot questSlot)
{
   check(GetOwner()->HasAuthority());
   if (UTATRootPlayerObjective* objective = GetObjectiveForSlot(questSlot))
   {
      return objective->AuthorityCheatComplete();
   }
}

bool UTATPlayerQuestComponent::IsQuestCompleteForMatchEnd(ETATPlayerQuestSlot slot, bool escaped, const FMatchPersistentData& matchData) const
{
   if (const UTATRootPlayerObjective* objective = GetObjectiveForSlot(slot))
   {
      return objective->IsCompleteForMatchEnd(escaped, matchData);
   }

   return false;
}

bool UTATPlayerQuestComponent::IsQuestObjectiveComplete(ETATPlayerQuestSlot questSlot) const
{
   return _GetObjectiveStateForSlot(questSlot).IsComplete;
}

FGameplayTag UTATPlayerQuestComponent::GetActiveQuestTag(ETATPlayerQuestSlot questSlot) const
{
   if (const UTATRootPlayerObjective* objective = GetObjectiveForSlot(questSlot))
   {
      return objective->QuestTag;
   }

   return {};
}

bool UTATPlayerQuestComponent::HasActiveQuest(ETATPlayerQuestSlot questSlot) const
{
   return GetObjectiveForSlot(questSlot) != nullptr;
}

void UTATPlayerQuestComponent::_BroadcastQuestObjectiveCompleteChanged(bool isComplete, ETATPlayerQuestSlot slot)
{
   OnQuestObjectiveCompleteChanged.Broadcast(isComplete, slot);
   
   if(GetOwner()->HasAuthority() && isComplete)
   {
      // Clamp match timer after completing mission
      // CONSIDER: Is there a better place to put this? (just in a listener?)
      if(slot == ETATPlayerQuestSlot::Mission)
      {
         if (auto* tatGameState = GetWorld()->GetGameState<ATATGameState>())
         {
            return tatGameState->AuthorityStartEndgame(ETATEndgameReason::Mission);
         }
      }
      if(auto* objectiveSubsystem = GetWorld()->GetSubsystem<UTATSharedObjectiveSubsystem>())
      {
         objectiveSubsystem->NotifySlotCompleted(slot);
      }
   }
}

void UTATPlayerQuestComponent::_OnRep_ActiveQuest()
{
   _OnActiveQuestsSet();
}

// Fine-ish to call repeatedly, if values end up the same, it won't fire events
void UTATPlayerQuestComponent::_OnActiveQuestsSet()
{
   _activeQuestTags.Reset();
   for(const UTATRootPlayerObjective* objective : _objectives)
   {
      if(objective)
      {
         _activeQuestTags.AddUnique(objective->QuestTag);
      }
   }
   
   if (_IsLocalPlayer())
   {
      if (auto* localPlayerStateSubsystem = GetWorld()->GetSubsystem<UTATLocalPlayerStateWorldSubsystem>())
      {
         localPlayerStateSubsystem->SetLocalQuests(GetActiveQuestTags());
      }
   }
}

void UTATPlayerQuestComponent::_OnRep_RelatedLootIds()
{
   if (_IsLocalPlayer())
   {
      if (auto* localPlayerStateSubsystem = GetWorld()->GetSubsystem<UTATLocalPlayerStateWorldSubsystem>())
      {
         localPlayerStateSubsystem->SetQuestRelatedLoot(_relatedLootIds);
      }
   }
}

UTATRootPlayerObjective* UTATPlayerQuestComponent::GetObjectiveForSlot(ETATPlayerQuestSlot slot) const
{
   for(UTATRootPlayerObjective* objective : _objectives)
   {
      if(objective && objective->Slot == slot)
      {
         return objective;
      }
   }
   return nullptr;
}

FTATQuestObjectiveState UTATPlayerQuestComponent::_GetObjectiveStateForSlot(ETATPlayerQuestSlot slot) const
{
   if (const UTATRootPlayerObjective* objective = GetObjectiveForSlot(slot))
   {
      return objective->GetState();
   }

   return FTATQuestObjectiveState();
}

void UTATPlayerQuestComponent::_OnRep_Objectives()
{
   _OnActiveQuestsSet();
}

bool UTATPlayerQuestComponent::_IsLocalPlayer() const
{
   switch(GetOwner()->GetLocalRole())
   {
   case ROLE_AutonomousProxy:
      return true;
   case ROLE_SimulatedProxy:
      return false;
   case ROLE_Authority:
   default:
      const APlayerState* playerState = GetOwner<APlayerState>();
      const APlayerController* controller = playerState ? playerState->GetPlayerController() : nullptr;
      return controller && controller->IsLocalController();
   }
}

TConstArrayView<FGameplayTag> UTATPlayerQuestComponent::GetActiveQuestTags() const
{
   return _activeQuestTags;
}

bool UTATPlayerQuestComponent::HasQuestRelatedLoot(FTATLootIdentifier lootId) const
{
   if(!lootId.IsValid())
   {
      return false;
   }

   return GetQuestRelatedLoot().Contains(lootId);
}

UTATPlayerQuestComponent::FQuestLootResult UTATPlayerQuestComponent::GetQuestRelatedLoot() const
{
   return _relatedLootIds;
}

void UTATPlayerQuestComponent::NotifyObjectiveComplete(bool complete, UTATRootPlayerObjective* objective)
{
   _BroadcastQuestObjectiveCompleteChanged(complete, objective->Slot);
}

void UTATPlayerQuestComponent::NotifyObjectiveProgress(int32 progress, UTATRootPlayerObjective* objective)
{
   OnQuestObjectiveProgressChanged.Broadcast(progress, objective->Slot);
}
