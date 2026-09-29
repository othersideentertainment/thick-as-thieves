// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask_EventSync.h"
#include "AbilityTask_LocalInputReleased.generated.h"


/// Simplified, local-only version of AbilityTask_WaitInputRelease. Provides a duration to limit the amount
/// of time the task will wait for input to be released. Always tests for the current state
/// when activated.
UCLASS()
class OSECORE_API UAbilityTask_LocalInputReleased : public UAbilityTask_EventSync
{
   GENERATED_BODY()

   /// Delegate called when the input is released
   UPROPERTY(BlueprintAssignable)
   FEventSyncDelegate OnReleased;

   /// Local callback method for the ability event delegate
   UFUNCTION()
   void OnReleasedCallback();
   
public:

   /// Constructor
   UAbilityTask_LocalInputReleased(const FObjectInitializer& objectInitializer);

   /// Waits for local input released until the time expires
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
   static UAbilityTask_LocalInputReleased* WaitForLocalInputReleased(UGameplayAbility* owningAbility, float duration = 0.05f);

protected:

   /// Called to trigger the actual task once the delegates have been set up
   virtual void Activate() override;

private:

   /// Handle for the abilities' input released delegate
   FDelegateHandle _delegateHandle;
};
