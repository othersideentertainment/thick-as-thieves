// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/OSEAbilityTask.h"
#include "AbilityTask_WaitCancelReleased.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWaitCancelReleasedDelegate);

UCLASS()
class OSECORE_API UAbilityTask_WaitCancelReleased : public UOSEAbilityTask
{
   GENERATED_UCLASS_BODY()

   UPROPERTY(BlueprintAssignable)
   FWaitCancelReleasedDelegate   OnCancelReleased;

   UFUNCTION()
   void OnCancelReleasedCallback();

   UFUNCTION()
   void OnLocalCancelReleasedCallback();

   UFUNCTION(BlueprintCallable, meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true", DisplayName = "Wait for Cancel Input Released"), Category = "Ability|Tasks")
   static UAbilityTask_WaitCancelReleased* WaitCancelReleased(UGameplayAbility* owningAbility);

   virtual void Activate() override;

protected:

   virtual void OnDestroy(bool abilityEnding) override;
   bool _registeredCallbacks;
};
