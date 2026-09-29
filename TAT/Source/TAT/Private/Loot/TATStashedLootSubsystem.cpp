// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Loot/TATStashedLootSubsystem.h"
   
// tat
#include "Developer/TATLootSettings.h"
#include "Developer/TATProjectSettings.h"
#include "Loot/TATTeamLootStash.h"
#include "Online/TATGameState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStashedLootSubsystem)


void UTATStashedLootSubsystem::_AuthorityReportStashedLoot(int value)
{
   const int prevTotal = _sealedStashedLootValue;
   _sealedStashedLootValue += value;
   
   const int endgameThreshold = UTATProjectSettings::Get().StashedLootValueToTriggerEndgame;
   if(prevTotal < endgameThreshold && _sealedStashedLootValue >= endgameThreshold)
   {
      if(ATATGameState* gameState = GetWorld()->GetGameState<ATATGameState>())
      {
         gameState->AuthorityStartEndgame(ETATEndgameReason::Loot);
      }
   }
}

void UTATStashedLootSubsystem::AuthorityAddStashedLoot(uint8 team, TConstArrayView<FTATLootIdentifier> loot)
{
   ATATTeamLootStash* teamStash = _AuthorityGetOrCreateStash(team);
   check(teamStash);
   const int valueToAdd = UTATLootSettings::Get().GetLootValue(this, loot);
   teamStash->AuthorityAddStashedLoot(valueToAdd, loot);
   _AuthorityReportStashedLoot(valueToAdd);
}

void UTATStashedLootSubsystem::NotifyTeamLootTotal(uint8 team, int32 totalValue)
{
   _stashValueByTeam.Add(team, totalValue);
   if (FOnTeamLootValueChanged* delegate = _valueListenersByTeam.Find(team))
   {
      delegate->Broadcast(totalValue);
   }
}

int32 UTATStashedLootSubsystem::GetStashedValueForTeam(uint8 team) const
{
   return _stashValueByTeam.FindRef(team);
}

TConstArrayView<FTATLootIdentifier> UTATStashedLootSubsystem::AuthorityGetStashedLootForTeam(uint8 team) const
{
   if (ATATTeamLootStash* found = _authorityStashByTeam.FindRef(team))
   {
      return found->AuthorityGetStashedLoot();
   }

   return TConstArrayView<FTATLootIdentifier>();
}

UTATStashedLootSubsystem::FOnTeamLootValueChanged& UTATStashedLootSubsystem::GetTeamValueListener(uint8 team)
{
   return _valueListenersByTeam.FindOrAdd(team);
}

void UTATStashedLootSubsystem::RemoveTeamValueListener(uint8 team, const UObject* userObject)
{
   if (FOnTeamLootValueChanged* found = _valueListenersByTeam.Find(team))
   {
      found->RemoveAll(userObject);
   }
}

bool UTATStashedLootSubsystem::DoesSupportWorldType(const EWorldType::Type worldType) const
{
   return worldType == EWorldType::Game || worldType == EWorldType::PIE;
}

ATATTeamLootStash* UTATStashedLootSubsystem::_AuthorityGetOrCreateStash(uint8 team)
{
   if(ATATTeamLootStash* found = _authorityStashByTeam.FindRef(team))
   {
      return found;
   }

   ATATTeamLootStash* teamStash = GetWorld()->SpawnActorDeferred<ATATTeamLootStash>(ATATTeamLootStash::StaticClass(), FTransform::Identity);
   teamStash->InitTeam(team);
   _authorityStashByTeam.Add(team, teamStash);
   UGameplayStatics::FinishSpawningActor(teamStash, FTransform::Identity);
   return teamStash;
}
