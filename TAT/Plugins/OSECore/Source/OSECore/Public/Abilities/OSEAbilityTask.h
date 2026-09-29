// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "OSEAbilityTask.generated.h"

/// Derived base gameplay ability task
UCLASS(Abstract, ClassGroup = (Ability))
class OSECORE_API UOSEAbilityTask : public UAbilityTask
{
   GENERATED_BODY()

public:

   /// Constructor
   UOSEAbilityTask(const FObjectInitializer& objectInitializer);

   /// Tick function for this task, if bTickingTask == true
   virtual void TickTask(float deltaTime) override;

protected:
   
   /// Called to trigger the actual task once the delegates have been set up
   virtual void Activate() override;

   /// End and CleanUp the task - may be called by the task itself or by the task owner if the owner is ending.
   /// IMPORTANT! Do NOT call directly! Call EndTask() or TaskOwnerEnded()
   /// IMPORTANT! When overriding this function make sure to call Super::OnDestroy(bOwnerFinished) as the last thing,
   ///            since the function internally marks the task as "Pending Kill", and this may interfere with internal BP mechanics
   virtual void OnDestroy(bool inOwnerFinished) override;
};
