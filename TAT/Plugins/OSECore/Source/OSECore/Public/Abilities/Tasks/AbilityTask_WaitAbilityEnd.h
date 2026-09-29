// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "Abilities/Tasks/AbilityTask.h"

#include "AbilityTask_WaitAbilityEnd.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWaitAbilityEndDelegate, UGameplayAbility*, endedAbility);

class AActor;

/**
 *   Waits for the actor to end another ability
 */
UCLASS()
class OSECORE_API UAbilityTask_WaitAbilityEnd : public UAbilityTask
{
   GENERATED_UCLASS_BODY()

   UPROPERTY(BlueprintAssignable)
   FWaitAbilityEndDelegate OnEnd;

   virtual void Activate() override;
   virtual void OnDestroy(bool abilityEnded) override;

   /** Wait until a new ability (of the same or different type) is ended. Only input based abilities will be counted unless IncludeTriggeredAbilities is true. */
   UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_WaitAbilityEnd* WaitForAbilityEnd(UGameplayAbility* owningAbility, FGameplayTag WithTag, FGameplayTag withoutTag, AActor* inOptionalExternalTarget = nullptr, bool includeTriggeredAbilities=false, bool triggerOnce=true);

   /** Wait until a new ability (of the same or different type) is ended. Only input based abilities will be counted unless IncludeTriggeredAbilities is true. Uses a tag requirements structure to filter abilities. */
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_WaitAbilityEnd* WaitForAbilityEndWithTagRequirements(UGameplayAbility* owningAbility, FGameplayTagRequirements tagRequirements, AActor* inOptionalExternalTarget = nullptr, bool includeTriggeredAbilities = false, bool triggerOnce = true);

   /** Wait until a new ability (of the same or different type) is ended. Only input based abilities will be counted unless IncludeTriggeredAbilities is true. */
   UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_WaitAbilityEnd* WaitForAbilityEnd_Query(UGameplayAbility* owningAbility, FGameplayTagQuery query, AActor* inOptionalExternalTarget = nullptr, bool includeTriggeredAbilities=false, bool triggerOnce=true);

protected:
   UFUNCTION()
   void _OnAbilityEnd(UGameplayAbility* ActivatedAbility);

   UAbilitySystemComponent* _GetTargetASC() const;
   void _SetOptionalExternalTarget(AActor* optionalExternalTarget);

   FDelegateHandle _onAbilityEndDelegateHandle;

   FGameplayTag _withTag;
   FGameplayTag _withoutTag;
   bool _includeTriggeredAbilities;
   bool _triggerOnce;
   FGameplayTagRequirements _tagRequirements;
   FGameplayTagQuery _query;
   bool _useExternalTarget = false;
   UPROPERTY()
   UAbilitySystemComponent* _optionalExternalTarget;
};
