// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "Abilities/OSEAbilityTypes.h"
#include "AbilityTask_WaitTargetEvent.generated.h"

/**
 * Modified version of UAbilityTask_WaitTargetData, it handles target actor pooling and will respond to externally generated target events that have an event tag
 */
UCLASS()
class OSECORE_API UAbilityTask_WaitTargetEvent : public UAbilityTask
{
   GENERATED_BODY()

   UPROPERTY(BlueprintAssignable)
   FWaitTargetEventDelegate ValidData;

   UPROPERTY(BlueprintAssignable)
   FWaitTargetEventDelegate Cancelled;

   /**
    * Starts a targeting task and wait for the actual targeting to happen. When the target data is generated depends on ConfirmationType
    *
    * @param   TaskInstanceName   Specify instance name for task, can be looked up later from ASC
    * @param   ConfirmationType   When to generate target data, rather it happens predicted or on server depends on ShouldProduceTargetDataOnServer
    * @param   Class            What class to use to perform target logic, can only be null for Custom and CustomMulti
    * @param   EventTag         If set, only custom events that match this tag will be handled. This will get passed through to result if not overriden by a custom event
    */
   UFUNCTION(BlueprintCallable, meta=(HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true", HideSpawnParms="Instigator"), Category="Ability|Tasks")
   static UAbilityTask_WaitTargetEvent* WaitTargetEvent(UGameplayAbility* owningAbility, FName taskInstanceName, FGameplayTag eventTag, TEnumAsByte<EGameplayTargetingConfirmation::Type> confirmationType, TSubclassOf<AGameplayAbilityTargetActor> Class);

   UFUNCTION(BlueprintCallable, meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"), Category = "Abilities")
   bool BeginSpawningActor(UGameplayAbility* owningAbility, TSubclassOf<AGameplayAbilityTargetActor> Class, AGameplayAbilityTargetActor*& SpawnedActor);

   UFUNCTION(BlueprintCallable, meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"), Category = "Abilities")
   void FinishSpawningActor(UGameplayAbility* owningAbility, AGameplayAbilityTargetActor* spawnedActor);

   virtual void Activate() override;

   UFUNCTION(BlueprintCallable, meta = (Category = "Gameplay Tasks"))
   virtual void ExternalCancel() override;

   UFUNCTION(BlueprintCallable, meta = (Category = "Gameplay Tasks"))
   virtual void ExternalConfirm(bool bEndTask) override;

   //produce validation data, for use with CustomMulti confirmation type
   UFUNCTION(BlueprintCallable, meta = (Category = "Gameplay Tasks"))
   virtual void ValidateTarget();

   UFUNCTION(BlueprintCallable, meta = (Category = "Gameplay Tasks"))
   AGameplayAbilityTargetActor* GetTargetActor()
   {
      return _targetActor;
   }

private:
   void OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& data);
   void OnTargetDataCancelledCallback(const FGameplayAbilityTargetDataHandle& data);
   

   void OnLocalTargetDataSetCallback(const FGameplayAbilityTargetDataHandle& data, FGameplayTag activationTag);
   void OnLocalTargetDataCancelledCallback();
   void OnTargetDataReplicatedCallback(const FGameplayAbilityTargetDataHandle& data, FGameplayTag activationTag);
   void OnTargetDataReplicatedCancelledCallback();

   bool ShouldSpawnTargetActor() const;
   void InitializeTargetActor(AGameplayAbilityTargetActor* spawnedActor);
   void FinalizeTargetActor(AGameplayAbilityTargetActor* spawnedActor);

   void RegisterTargetDataCallbacks();

   virtual void OnDestroy(bool abilityEnded) override;

   bool ShouldReplicateDataToServer() const;

protected:

   UPROPERTY()
   TSubclassOf<AGameplayAbilityTargetActor> _targetClass;

   /** The TargetActor that we spawned */
   UPROPERTY()
   AGameplayAbilityTargetActor* _targetActor;

   TEnumAsByte<EGameplayTargetingConfirmation::Type> _confirmationType;

   FDelegateHandle _onTargetDataReplicatedCallbackDelegateHandle;

   FGameplayTag _eventTag;
};

