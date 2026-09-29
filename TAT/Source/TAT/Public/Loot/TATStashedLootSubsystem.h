// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Subsystems/WorldSubsystem.h"

#include "TATStashedLootSubsystem.generated.h"

struct FTATLootIdentifier;
class ATATTeamLootStash;

// A subsystem to keep track of stashed loot by team
//
// Also tracks cumulative stashed loot value across all players in order
// to trigger the endgame.
UCLASS()
class TAT_API UTATStashedLootSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:
   void AuthorityAddStashedLoot(uint8 team, TConstArrayView<FTATLootIdentifier> loot);

   // called by replicated loot stash for teams
   void NotifyTeamLootTotal(uint8 team, int32 totalValue);

   int32 GetStashedValueForTeam(uint8 team) const;
   TConstArrayView<FTATLootIdentifier> AuthorityGetStashedLootForTeam(uint8 team) const;

   DECLARE_MULTICAST_DELEGATE_OneParam(FOnTeamLootValueChanged, int32);
   FOnTeamLootValueChanged& GetTeamValueListener(uint8 team);
   void RemoveTeamValueListener(uint8 team, const UObject* object);
   

protected:
   virtual bool DoesSupportWorldType(const EWorldType::Type worldType) const override;

private:
   void _AuthorityReportStashedLoot(int value);
   ATATTeamLootStash* _AuthorityGetOrCreateStash(uint8 team);

   
   int32 _sealedStashedLootValue = 0;

   UPROPERTY(Transient)
   TMap<uint8, TObjectPtr<ATATTeamLootStash>> _authorityStashByTeam;

   UPROPERTY(Transient)
   TMap<uint8, int32> _stashValueByTeam;
   TMap<uint8, FOnTeamLootValueChanged> _valueListenersByTeam;
};
