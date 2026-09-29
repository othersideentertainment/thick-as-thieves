// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATPlayerQuestSlot.h"

// ue
#include "Subsystems/WorldSubsystem.h"

#include "TATSharedObjectiveSubsystem.generated.h"

class UTATPlayerObjective;

struct FTATSharedObjectiveTeam;

struct FTATSharedObjectiveEntry
{
   TWeakObjectPtr<UTATPlayerObjective> Objective;
   int32 PlayerId = 0; //< PlayerId that matches playerId in PlayerState

   bool operator==(TWeakObjectPtr<UTATPlayerObjective> otherObjective) const
   {
      return Objective == otherObjective;
   }

   bool operator==(int32 otherPlayerId) const
   {
      return PlayerId == otherPlayerId;
   }
};

struct FTATSharedObjective
{
   TArray<FTATSharedObjectiveEntry> ByPlayer;

   void Add(const FTATSharedObjectiveEntry& entry);
   TWeakObjectPtr<UTATPlayerObjective> RemovePlayer(int32 playerId);

   void RefreshCompleteness();
};

struct FTATSharedObjectiveTeam
{
   TMap<uint64, FTATSharedObjective> Objectives;

   void Add(const FTATSharedObjectiveEntry& entry);
};

// An authority-only subsystem that combines objective progress from allies
UCLASS()
class TAT_API UTATSharedObjectiveSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:
   virtual bool ShouldCreateSubsystem(UObject* outer) const override;

   void RegisterObjective(int32 playerId, UTATPlayerObjective* objective);

   void SetPlayerTeam(int32 playerId, uint8 team);
   void UnregisterPlayer(int32 playerId);

   // Separately, some first-time-completion stuff
   void NotifySlotCompleted(ETATPlayerQuestSlot slot);
   FSimpleMulticastDelegate& OnSlotEverCompleted(ETATPlayerQuestSlot slot);
   bool WasSlotEverCompleted(ETATPlayerQuestSlot slot) const;

private:
   void _RefreshObjective(int32 playerId, uint64 objectiveId);
   
   TMap<int32, uint8> _playerToTeam;
   TMap<uint8, FTATSharedObjectiveTeam> _teams;

   bool _everCompletedSlots[static_cast<int32>(ETATPlayerQuestSlot::MAX)] = {false};
   FSimpleMulticastDelegate _onEverCompleted[static_cast<int32>(ETATPlayerQuestSlot::MAX)];
};
