// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Traps/Old/TrapTriggerBase.h"

#include "PressurePlate.generated.h"


/// A trap trigger that triggers when an applicable actor steps off it
UCLASS()
class TAT_API APressurePlate_Old : public ATrapTriggerBase_Old
{
   GENERATED_BODY()
   
public:   
   // Sets default values for this actor's properties
   APressurePlate_Old();

   // If we need to only do this for specific components, can change this back into
   // BlueprintCallable functions that blueprint subclasses can call explicitly, or
   // explicitly listen the overlaps with root component? (may need to be more specific,
   // as actors may have have multiple relevant components, which the actor granularity
   // handles)
   virtual void NotifyActorBeginOverlap(AActor* otherActor) override;
   virtual void NotifyActorEndOverlap(AActor* otherActor) override;

protected:
   UFUNCTION(BlueprintNativeEvent, BlueprintPure, meta = (BlueprintProtected))
   bool CanBeTriggeredByActor(AActor* actor) const;
   virtual bool CanBeTriggeredByActor_Implementation(AActor* actor) const { return true; }

   virtual ETrapTriggerState GetStateAfterReset() const override;

private:
   // server-only
   UPROPERTY(Transient)
   TArray<AActor*> _actorsOnPlate;

   // Trigger the trap on enter, rather than exiting the plate, even though that is the main difference
   UPROPERTY(EditAnywhere, Category = Config, AdvancedDisplay)
   bool _triggerOnEnter;
};
