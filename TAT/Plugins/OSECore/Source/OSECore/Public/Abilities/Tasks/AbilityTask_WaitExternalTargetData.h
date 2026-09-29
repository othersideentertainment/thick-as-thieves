// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_WaitExternalTargetData.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWaitExternalTargetDataDelegate, const FGameplayAbilityTargetDataHandle&, Data);

/**
 * Stripped down from UAbilityTask_WaitTargetData, it does not spawn an actor for targeting, and just uses an externally provided one
 */
UCLASS()
class OSECORE_API UAbilityTask_WaitExternalTargetData : public UAbilityTask
{
   GENERATED_BODY()

   UPROPERTY(BlueprintAssignable)
   FWaitExternalTargetDataDelegate ValidData;

   UPROPERTY(BlueprintAssignable)
   FWaitExternalTargetDataDelegate Cancelled;

   // Waits for the provided target data
   // On a non-local server, it will throw its away and wait for target data from the client
   UFUNCTION(BlueprintCallable, meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true", HideSpawnParms = "Instigator"), Category = "Ability|Tasks")
   static UAbilityTask_WaitExternalTargetData* WaitProvidedTargetData(UGameplayAbility* owningAbility, FName taskInstanceName, const FGameplayAbilityTargetDataHandle& handle);

   virtual void Activate() override;

   /** Called when the ability is asked to cancel from an outside node. What this means depends on the individual task. By default, this does nothing other than ending the task. */
   virtual void ExternalCancel() override;

public:
   // maybe make public again when there is a non-immediate form?
   void TryProvideTarget(const FGameplayAbilityTargetDataHandle& data);

private:
   void RegisterTargetDataCallbacks();

   bool ShouldProduceLocalTarget() const;

   void OnLocalTargetDataReady(const FGameplayAbilityTargetDataHandle& data);
   void OnLocalTargetDataCancelled(const FGameplayAbilityTargetDataHandle& data);

   UFUNCTION()
   void OnTargetDataReplicatedCallback(const FGameplayAbilityTargetDataHandle& data, FGameplayTag activationTag);
   UFUNCTION()
   void OnTargetDataReplicatedCancelledCallback();

protected:
   FDelegateHandle _onTargetDataReplicatedCallbackDelegateHandle;

   FGameplayAbilityTargetDataHandle _providedTargetData;
   bool _targetDataProvided;
};

