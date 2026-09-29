// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/TATSharedObjectiveSubsystem.h"

// tat
#include "Quests/TATPlayerObjective.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSharedObjectiveSubsystem)


void FTATSharedObjective::Add(const FTATSharedObjectiveEntry& entry)
{
   ByPlayer.Add(entry);
   RefreshCompleteness();
}

TWeakObjectPtr<UTATPlayerObjective> FTATSharedObjective::RemovePlayer(int32 playerId)
{
   int32 index = ByPlayer.IndexOfByKey(playerId);
   if (index >= 0)
   {
      TWeakObjectPtr<UTATPlayerObjective> found = ByPlayer[index].Objective;
      ByPlayer.RemoveAtSwap(index, EAllowShrinking::No);
      RefreshCompleteness();
      return found;
   }

   return nullptr;
}

void FTATSharedObjective::RefreshCompleteness()
{
   uint32 completeMask = 0;
   for (int i = 0; i < ByPlayer.Num(); i++)
   {
      if (const UTATPlayerObjective* objective = ByPlayer[i].Objective.Get())
      {
         completeMask |= static_cast<uint32>(objective->IsCompleteSelf()) << i;
      }
   }

   // And then set shared
   for (int i = 0; i < ByPlayer.Num(); i++)
   {
      if (UTATPlayerObjective* objective = ByPlayer[i].Objective.Get())
      {
         const bool hadCompleteAlly = (completeMask & ~(1u << i)) != 0;
         objective->AuthoritySetAllyObjectiveState(FTATQuestObjectiveState{
            .IsComplete = hadCompleteAlly
         });
      }
   }

}

void FTATSharedObjectiveTeam::Add(const FTATSharedObjectiveEntry& entry)
{
   Objectives.FindOrAdd(entry.Objective->AuthorityId).Add(entry);
}

bool UTATSharedObjectiveSubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   if (!Super::ShouldCreateSubsystem(outer))
   {
      return false;
   }

   UWorld* world = CastChecked<UWorld>(outer);
   return !world->IsNetMode(NM_Client);
}

void UTATSharedObjectiveSubsystem::RegisterObjective(int32 playerId, UTATPlayerObjective* objective)
{
   check(objective);
   const uint8 teamId = _playerToTeam.FindChecked(playerId);
   
   FTATSharedObjectiveTeam& team = _teams.FindOrAdd(teamId);
   team.Add({
      .Objective = objective,
      .PlayerId = playerId
   });

   objective->OnAuthorityCompleteSelfChanged.AddUObject(this, &ThisClass::_RefreshObjective, playerId, objective->AuthorityId);
}

void UTATSharedObjectiveSubsystem::SetPlayerTeam(int32 playerId, uint8 newTeamId)
{
   if (const uint8* oldTeamIdPtr = _playerToTeam.Find(playerId))
   {
      const uint8 oldTeamId = *oldTeamIdPtr;
      if (oldTeamId == newTeamId)
      {
         return;
      }

      // Transfer any old objectives to the new team
      if (FTATSharedObjectiveTeam* oldTeam = _teams.Find(oldTeamId))
      {
         // take objectives from old team
         TArray<TWeakObjectPtr<UTATPlayerObjective>, TInlineAllocator<8>> removedObjectives;
         for (auto& pair : oldTeam->Objectives)
         {
            TWeakObjectPtr<UTATPlayerObjective> removed = pair.Value.RemovePlayer(playerId);
            if (removed.IsValid())
            {
               removedObjectives.Add(removed);
            }
            // TODO: remove if empty?
         }
         if (oldTeam->Objectives.IsEmpty())
         {
            _teams.Remove(oldTeamId);
         }

         // move objectives to new team
         if (removedObjectives.Num())
         {
            FTATSharedObjectiveTeam& newTeam = _teams.FindOrAdd(newTeamId);
            for (TWeakObjectPtr<UTATPlayerObjective> objective : removedObjectives)
            {
               newTeam.Add({
                  .Objective = objective,
                  .PlayerId = playerId
               });
            }
         }
      }
   }

   // Update team map
   _playerToTeam.Add(playerId, newTeamId);
}

void UTATSharedObjectiveSubsystem::UnregisterPlayer(int32 playerId)
{
   const uint8* teamIdPtr = _playerToTeam.Find(playerId);
   if (teamIdPtr == nullptr)
   {
      return;
   }

   if (FTATSharedObjectiveTeam* teamEntry = _teams.Find(*teamIdPtr))
   {
      for (auto& pair : teamEntry->Objectives)
      {
         TWeakObjectPtr<UTATPlayerObjective> removed = pair.Value.RemovePlayer(playerId);
         if (UTATPlayerObjective* removedPtr = removed.Get())
         {
            removedPtr->OnAuthorityCompleteSelfChanged.RemoveAll(this);
         }
      }
   }
   _playerToTeam.Remove(playerId);
}

void UTATSharedObjectiveSubsystem::NotifySlotCompleted(ETATPlayerQuestSlot slot)
{
   check(slot < ETATPlayerQuestSlot::MAX);
   const int32 index = static_cast<int32>(slot);
   if(!_everCompletedSlots[index])
   {
      _everCompletedSlots[index] = true;
      _onEverCompleted[index].Broadcast();
   }
}

FSimpleMulticastDelegate& UTATSharedObjectiveSubsystem::OnSlotEverCompleted(ETATPlayerQuestSlot slot)
{
   check(slot < ETATPlayerQuestSlot::MAX);
   const int32 index = static_cast<int32>(slot);
   return _onEverCompleted[index];
}

bool UTATSharedObjectiveSubsystem::WasSlotEverCompleted(ETATPlayerQuestSlot slot) const
{
   check(slot < ETATPlayerQuestSlot::MAX);
   const int32 index = static_cast<int32>(slot);
   return _everCompletedSlots[index];
}

void UTATSharedObjectiveSubsystem::_RefreshObjective(int32 playerId, uint64 objectiveId)
{
   const uint8* teamIdPtr = _playerToTeam.Find(playerId);
   if (teamIdPtr == nullptr)
   {
      return;
   }

   FTATSharedObjectiveTeam* teamEntry = _teams.Find(*teamIdPtr);
   if (teamEntry == nullptr)
   {
      return;
   }

   FTATSharedObjective* cluster = teamEntry->Objectives.Find(objectiveId);
   if (cluster == nullptr)
   {
      return;
   }

   cluster->RefreshCompleteness();
}
