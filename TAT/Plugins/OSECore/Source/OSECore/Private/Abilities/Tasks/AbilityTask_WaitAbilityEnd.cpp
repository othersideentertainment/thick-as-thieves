// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Tasks/AbilityTask_WaitAbilityEnd.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_WaitAbilityEnd)


UAbilityTask_WaitAbilityEnd::UAbilityTask_WaitAbilityEnd(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{

}

UAbilityTask_WaitAbilityEnd* UAbilityTask_WaitAbilityEnd::WaitForAbilityEnd(UGameplayAbility* owningAbility, FGameplayTag inWithTag, FGameplayTag inWithoutTag, AActor* inOptionalExternalTarget, bool inIncludeTriggeredAbilities, bool inTriggerOnce)
{
   UAbilityTask_WaitAbilityEnd* myObj = NewAbilityTask<UAbilityTask_WaitAbilityEnd>(owningAbility);
   myObj->_withTag = inWithTag;
   myObj->_withoutTag = inWithoutTag;
   myObj->_includeTriggeredAbilities = inIncludeTriggeredAbilities;
   myObj->_triggerOnce = inTriggerOnce;
   myObj->_SetOptionalExternalTarget(inOptionalExternalTarget);
   return myObj;
}

UAbilityTask_WaitAbilityEnd* UAbilityTask_WaitAbilityEnd::WaitForAbilityEndWithTagRequirements(UGameplayAbility* owningAbility, FGameplayTagRequirements tagRequirements, AActor* inOptionalExternalTarget, bool inIncludeTriggeredAbilities, bool inTriggerOnce)
{
   UAbilityTask_WaitAbilityEnd* myObj = NewAbilityTask<UAbilityTask_WaitAbilityEnd>(owningAbility);
   myObj->_tagRequirements = tagRequirements;
   myObj->_includeTriggeredAbilities = inIncludeTriggeredAbilities;
   myObj->_triggerOnce = inTriggerOnce;
   myObj->_SetOptionalExternalTarget(inOptionalExternalTarget);
   return myObj;
}

UAbilityTask_WaitAbilityEnd* UAbilityTask_WaitAbilityEnd::WaitForAbilityEnd_Query(UGameplayAbility* owningAbility, FGameplayTagQuery query, AActor* inOptionalExternalTarget, bool inIncludeTriggeredAbilities, bool inTriggerOnce)
{
   UAbilityTask_WaitAbilityEnd* myObj = NewAbilityTask<UAbilityTask_WaitAbilityEnd>(owningAbility);
   myObj->_query = query;
   myObj->_includeTriggeredAbilities = inIncludeTriggeredAbilities;
   myObj->_triggerOnce = inTriggerOnce;
   myObj->_SetOptionalExternalTarget(inOptionalExternalTarget);
   return myObj;
}

void UAbilityTask_WaitAbilityEnd::Activate()
{
   if (UAbilitySystemComponent* asc = _GetTargetASC())
   {
      _onAbilityEndDelegateHandle = asc->AbilityEndedCallbacks.AddUObject(this, &UAbilityTask_WaitAbilityEnd::_OnAbilityEnd);
   }
}

void UAbilityTask_WaitAbilityEnd::_OnAbilityEnd(UGameplayAbility* endedAbility)
{
   if (!_includeTriggeredAbilities && endedAbility->IsTriggered())
   {
      return;
   }

   const FGameplayTagContainer& abilityTags = endedAbility->GetAssetTags();

   if (_tagRequirements.IsEmpty())
   {
      if ((_withTag.IsValid() && !abilityTags.HasTag(_withTag)) ||
         (_withoutTag.IsValid() && abilityTags.HasTag(_withoutTag)))
      {
         // Failed tag check
         return;
      }
   }
   else
   {
      if (!_tagRequirements.RequirementsMet(abilityTags))
      {
         // Failed tag check
         return;
      }
   }

   if (_query.IsEmpty() == false)
   {
      if (_query.Matches(abilityTags) == false)
      {
         // Failed query
         return;
      }
   }

   if (ShouldBroadcastAbilityTaskDelegates())
   {
      OnEnd.Broadcast(endedAbility);
   }

   if (_triggerOnce)
   {
      EndTask();
   }
}

void UAbilityTask_WaitAbilityEnd::OnDestroy(bool abilityEnded)
{
   if (UAbilitySystemComponent* asc = _GetTargetASC())
   {
      asc->AbilityEndedCallbacks.Remove(_onAbilityEndDelegateHandle);
   }

   Super::OnDestroy(abilityEnded);
}

UAbilitySystemComponent* UAbilityTask_WaitAbilityEnd::_GetTargetASC() const
{
   if (_useExternalTarget)
   {
      return _optionalExternalTarget;
   }

   return AbilitySystemComponent.Get();
}

void UAbilityTask_WaitAbilityEnd::_SetOptionalExternalTarget(AActor* optionalExternalTarget)
{
   if (optionalExternalTarget)
   {
      _useExternalTarget = true;
      _optionalExternalTarget = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(optionalExternalTarget);
   }
}

