// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameFramework/Info.h"

#include "TATTeamLootStash.generated.h"


struct FTATLootIdentifier;

// A replicated actor that stores the stashed loot for a given team
UCLASS()
class TAT_API ATATTeamLootStash : public AInfo
{
   GENERATED_BODY()

public:
   // Sets default values for this actor's properties
   ATATTeamLootStash();
   virtual void BeginReplication() override;

   void InitTeam(uint8 team) { _team = team; }
   uint8 GetTeam() const { return _team; }

   void AuthorityAddStashedLoot(int valueToAdd, TConstArrayView<FTATLootIdentifier> stashedLoot);
   TConstArrayView<FTATLootIdentifier> AuthorityGetStashedLoot() const {
      check(HasAuthority());
      return _stashedLoot;
   }

private:
   void _NotifyLootValue();
   
   UFUNCTION()
   void _OnRep_TotalValue();
   
   UPROPERTY(Transient, Replicated)
   uint8 _team = 0;

   UPROPERTY(Transient, ReplicatedUsing = _OnRep_TotalValue)
   int32 _totalValue = 0;

   UPROPERTY(Transient)
   TArray<FTATLootIdentifier> _stashedLoot;
};
