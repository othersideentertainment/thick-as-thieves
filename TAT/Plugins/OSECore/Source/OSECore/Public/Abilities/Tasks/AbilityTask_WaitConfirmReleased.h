// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/OSEAbilityTask.h"
#include "AbilityTask_WaitConfirmReleased.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWaitConfirmReleasedDelegate);

UCLASS()
class OSECORE_API UAbilityTask_WaitConfirmReleased : public UOSEAbilityTask
{
   GENERATED_UCLASS_BODY()

   UPROPERTY(BlueprintAssignable)
   FWaitConfirmReleasedDelegate OnConfirmReleased;

   UFUNCTION()
   void OnConfirmReleasedCallback();

   UFUNCTION()
   void OnLocalConfirmReleasedCallback();

   UFUNCTION(BlueprintCallable, meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true", DisplayName = "Wait for Confirm Input Released"), Category = "Ability|Tasks")
   static UAbilityTask_WaitConfirmReleased* WaitConfirmReleased(UGameplayAbility* owningAbility);

   virtual void Activate() override;

protected:

   virtual void OnDestroy(bool abilityEnding) override;
   bool _registeredCallbacks;
};
