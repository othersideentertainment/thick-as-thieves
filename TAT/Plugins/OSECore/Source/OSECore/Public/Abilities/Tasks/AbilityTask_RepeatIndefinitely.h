// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_RepeatIndefinitely.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRepeatedIndefinitelyActionDelegate);

UCLASS()
class OSECORE_API UAbilityTask_RepeatIndefinitely : public UAbilityTask
{
   GENERATED_BODY()
   
   UPROPERTY(BlueprintAssignable)
   FRepeatedIndefinitelyActionDelegate   OnPerformAction;

   /// Start a task that repeats an action or set of actions.
   /// Use with care, but often better than a delay loop
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks|OSE", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_RepeatIndefinitely* RepeatActionIndefinitely(UGameplayAbility* owningAbility, float timeBetweenActions, bool jitterStartTime = false);

   virtual void Activate() override;

private:
   void _PerformAction();

protected:
   float _timeBetweenActions;
   FTimerHandle _timerHandle;
   bool _jitterStartTime = false;

   virtual void OnDestroy(bool abilityIsEnding) override;
};
