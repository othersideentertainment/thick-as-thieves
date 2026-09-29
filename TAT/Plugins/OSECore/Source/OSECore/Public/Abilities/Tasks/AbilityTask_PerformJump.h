// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask_TraversalBase.h"
#include "AbilityTask_PerformJump.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPerformJumpTaskDelegate, float, ElapsedTime);

UCLASS()
class OSECORE_API UAbilityTask_PerformJump : public UAbilityTask_TraversalBase
{
   GENERATED_BODY()

   /// Delegate called when the task is cancelled early (a jump couldn't be started, or was aborted)
   UPROPERTY(BlueprintAssignable)
   FPerformJumpTaskDelegate OnCancelled;

   /// Delegate called when the task is finished (a jump was started and completed)
   UPROPERTY(BlueprintAssignable)
   FPerformJumpTaskDelegate OnFinished;

public:

   /// Constructor
   UAbilityTask_PerformJump(const FObjectInitializer& objectInitializer);

   /// Perform a jump
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks|Traversal", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
   static UAbilityTask_PerformJump* PerformJump(UGameplayAbility* owningAbility, float duration);

   /// Perform a jump that will use the maximum hold time
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks|Traversal", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
   static UAbilityTask_PerformJump* PerformJumpMaxHold(UGameplayAbility* owningAbility);

   /// Tick function for this task, if bTickingTask == true
   virtual void TickTask(float deltaTime) override;

protected:

   /// Called to trigger the actual task once the delegates have been set up
   virtual void Activate() override;

   /// End and CleanUp the task - may be called by the task itself or by the task owner if the owner is ending.
   virtual void OnDestroy(bool inOwnerFinished) override;

private:

   /// Called to stop the jump
   void OnJumpFinished();

   /// Timer variables
   float _duration;
   float _elapsed;

   /// Set to true if we initiated a jump (and thus need to stop it)
   bool _initiatedJump;

   /// Set to true if the duration should be set to the max jump hold time
   bool _useMaxDuration;
};
