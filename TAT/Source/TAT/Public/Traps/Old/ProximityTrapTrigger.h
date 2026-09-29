// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Traps/Old/TrapTriggerBase.h"
#include "ProximityTrapTrigger.generated.h"

/// A Simple trap trigger that triggers on overlap
UCLASS()
class TAT_API AProximityTrapTrigger_Old : public ATrapTriggerBase_Old
{
   GENERATED_BODY()

public:
   // If we need to only do this for specific components, can change this back into
   // BlueprintCallable functions that blueprint subclasses can call explicitly, or
   // explicitly listen the overlaps with root component?
   virtual void NotifyActorBeginOverlap(AActor* otherActor) override;

protected:
   UFUNCTION(BlueprintNativeEvent, BlueprintPure, meta = (BlueprintProtected))
   bool CanBeTriggeredByActor(AActor* actor) const;
   virtual bool CanBeTriggeredByActor_Implementation(AActor* actor) const { return true; }
};
