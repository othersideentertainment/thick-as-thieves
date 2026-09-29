// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_WaitInputWithTimeout.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInputWithTimeoutDelegate, float, TimeHeld, bool, bTimedOut);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInputWithTimeoutBailDelegate);

/**
 * An ability task to wait for input press/release with a timeout
 */
UCLASS()
class OSECORE_API UAbilityTask_WaitInputWithTimeout : public UAbilityTask
{
   GENERATED_UCLASS_BODY()

   UPROPERTY(BlueprintAssignable)
   FInputWithTimeoutDelegate   OnInput;

   UPROPERTY(BlueprintAssignable)
   FInputWithTimeoutDelegate   OnTimeOut;

   // this may be called after the ability has ended, so use this with care
   UPROPERTY(BlueprintAssignable, AdvancedDisplay)
   FInputWithTimeoutBailDelegate   OnAbilityEnded;

   virtual void Activate() override;

   /** Wait until the user presses the input button for this ability's activation. Returns time from hitting this node, till release. Will return 0 if input was already released. */
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "owningAbility", DefaultToSelf = "owningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_WaitInputWithTimeout* WaitInputPressWithTimeout(UGameplayAbility* owningAbility, float timeoutSeconds = -1, bool testAlreadyPressed = false, bool useClientTime = false);

   /** Wait until the user releases the input button for this ability's activation. Returns time from hitting this node, till release. Will return 0 if input was already released. */
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "owningAbility", DefaultToSelf = "owningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_WaitInputWithTimeout* WaitInputReleaseWithTimeout(UGameplayAbility* owningAbility, float timeoutSeconds = -1, bool testAlreadyReleased = false, bool useClientTime = false);

   /** Wait until the user presses the confirm button for this ability's activation. Returns time from hitting this node, till release. Will return 0 if input was already released. */
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "owningAbility", DefaultToSelf = "owningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_WaitInputWithTimeout* WaitConfirmPressWithTimeout(UGameplayAbility* owningAbility, float timeoutSeconds = -1, bool useClientTime = false);

   /** Wait until the user releases the confirm button for this ability's activation. Returns time from hitting this node, till release. Will return 0 if input was already released. */
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "owningAbility", DefaultToSelf = "owningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_WaitInputWithTimeout* WaitConfirmReleaseWithTimeout(UGameplayAbility* owningAbility, float timeoutSeconds = -1, bool useClientTime = false);

   /** Wait until the user presses the cancel button for this ability's activation. Returns time from hitting this node, till release. Will return 0 if input was already released. */
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "owningAbility", DefaultToSelf = "owningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_WaitInputWithTimeout* WaitCancelPressWithTimeout(UGameplayAbility* owningAbility, float timeoutSeconds = -1, bool useClientTime = false);

   /** Wait until the user releases the cancel button for this ability's activation. Returns time from hitting this node, till release. Will return 0 if input was already released. */
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "owningAbility", DefaultToSelf = "owningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_WaitInputWithTimeout* WaitCancelReleaseWithTimeout(UGameplayAbility* owningAbility, float timeoutSeconds = -1, bool useClientTime = false);


private:
   bool _ShouldCompleteImmediately() const;
   void _OnTimeFinish();
   void _HandleComplete(float ElapsedTime, bool bTimedOut);

   UFUNCTION()
   void _OnInputCallback();
   virtual void OnDestroy(bool abilityEnded) override;

private:
   float _startTime;
   float _timeoutDuration;
   FDelegateHandle _delegateHandle;
   FTimerHandle _timerHandle;

   bool _testInitialState;
   bool _useClientTime;
   bool _alreadyNotified;
   TEnumAsByte<EAbilityGenericReplicatedEvent::Type> _eventType;
};
