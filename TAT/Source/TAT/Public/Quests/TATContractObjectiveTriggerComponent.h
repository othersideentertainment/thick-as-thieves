// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "TATContractObjectiveTriggerComponent.generated.h"

// A convenience component for triggering something when
// any player has completed an objective.
//
// Should only be used for contracts, since missions should
// use the TATEndgameActionComponent instead.
//
// Currently only notifying on authority, and leaving replication
// to the user.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TAT_API UTATContractObjectiveTriggerComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATContractObjectiveTriggerComponent();

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnObjectiveComplete);
   UPROPERTY(BlueprintAssignable)
   FOnObjectiveComplete AuthorityOnObjectiveComplete;

protected:
   virtual void BeginPlay() override;
};
