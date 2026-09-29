// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask_EventSync.h"
#include "AbilityTask_LocalInputPressed.generated.h"


/// Simplified, local-only version of AbilityTask_WaitInputPress. Provides a duration to limit the amount
/// of time the task will wait for input to be Pressed. Always tests for the current state
/// when activated.
UCLASS()
class OSECORE_API UAbilityTask_LocalInputPressed : public UAbilityTask_EventSync
{
   GENERATED_BODY()

   /// Delegate called when the input is pressed
   UPROPERTY(BlueprintAssignable)
   FEventSyncDelegate OnPressed;

   /// Local callback method for the ability event delegate
   UFUNCTION()
   void OnPressedCallback();
   
public:

   /// Constructor
   UAbilityTask_LocalInputPressed(const FObjectInitializer& objectInitializer);

   /// Waits for local input pressed until the time expires
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
   static UAbilityTask_LocalInputPressed* WaitForLocalInputPressed(UGameplayAbility* owningAbility, float duration = 0.05f);

protected:

   /// Called to trigger the actual task once the delegates have been set up
   virtual void Activate() override;

private:

   /// Handle for the abilities' input pressed delegate
   FDelegateHandle _delegateHandle;
};
