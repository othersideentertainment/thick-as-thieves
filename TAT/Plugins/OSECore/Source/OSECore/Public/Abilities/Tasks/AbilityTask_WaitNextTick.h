// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_WaitNextTick.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWaitTickDelegate, int32, elapsedTicks, float, elapsedTime);

UCLASS()
class OSECORE_API UAbilityTask_WaitNextTick : public UAbilityTask
{
   GENERATED_BODY()

   /// Delegate called when waiting is complete
   UPROPERTY(BlueprintAssignable)
   FWaitTickDelegate OnCompleted;

public:

   /// Constructor
   UAbilityTask_WaitNextTick(const FObjectInitializer& objectInitializer);

   /// Waits the specified number of ticks
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks|OSE", meta = (DisplayName = "Wait Next Tick(s) Count", HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_WaitNextTick* CreateWaitNextTickCount(UGameplayAbility* owningAbility, int32 numTicksToWait);

   /// Waits a single tick
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks|OSE", meta = (DisplayName = "Wait Next Tick", HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_WaitNextTick* CreateWaitNextTick(UGameplayAbility* owningAbility);

   /// Calls back each tick till we're asked to end
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks|OSE", meta = (DisplayName = "Tick Each Frame", HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_WaitNextTick* CreateTickForeverTask(UGameplayAbility* owningAbility);

   /// Tick function for this task, if bTickingTask == true
   virtual void TickTask(float DeltaTime) override;

private:

   // Tick variables
   int32 _ticksToWait;
   int32 _ticksElapsed;
   float _timeElapsed;
   bool _endAutomatically;
};
