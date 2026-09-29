// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Loot/TATTeamLootStash.h"

// tat
#include "Developer/TATLootSettings.h"
#include "Loot/TATStashedLootSubsystem.h"
#include "Net/TATIrisGroupSubsystem.h"

// ue
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTeamLootStash)

// Sets default values
ATATTeamLootStash::ATATTeamLootStash()
{
   PrimaryActorTick.bCanEverTick = false;
   bReplicates = true;
}

void ATATTeamLootStash::AuthorityAddStashedLoot(int valueToAdd, TConstArrayView<FTATLootIdentifier> stashedLoot)
{
   check(HasAuthority());

   _totalValue += valueToAdd;
   _stashedLoot.Append(stashedLoot);
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _totalValue, this);

   _NotifyLootValue();
}

void ATATTeamLootStash::_NotifyLootValue()
{
   if (UTATStashedLootSubsystem* stashSubsystem = GetWorld()->GetSubsystem<UTATStashedLootSubsystem>())
   {
      stashSubsystem->NotifyTeamLootTotal(_team, _totalValue);
   }
}

void ATATTeamLootStash::_OnRep_TotalValue()
{
   _NotifyLootValue();
}

void ATATTeamLootStash::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;
   
   params.Condition = COND_InitialOnly;
   DOREPLIFETIME_WITH_PARAMS(ThisClass, _team, params);

   params.Condition = COND_None;
   DOREPLIFETIME_WITH_PARAMS(ThisClass, _totalValue, params);
}

void ATATTeamLootStash::BeginReplication()
{
   Super::BeginReplication();

   if(UTATIrisGroupSubsystem* groupSubsystem = GetWorld()->GetSubsystem<UTATIrisGroupSubsystem>())
   {
      groupSubsystem->ForTeam(_team).AddActorToGroup(this);
   }
}
