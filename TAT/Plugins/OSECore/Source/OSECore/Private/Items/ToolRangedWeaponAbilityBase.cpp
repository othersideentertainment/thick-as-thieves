// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/ToolRangedWeaponAbilityBase.h"

// ose
#include "Items/ToolRangedWeaponComponent.h"
#include "Items/ToolSetInterface.h"
#include "Items/ToolSetSystemInterface.h"

// ue4
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ToolRangedWeaponAbilityBase)

UToolRangedWeaponAbilityBase::UToolRangedWeaponAbilityBase()
   : Super()
{
   ActivationRequiresController = true;
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
   InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UToolRangedWeaponAbilityBase::CanActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayTagContainer* sourceTags, const FGameplayTagContainer* targetTags, FGameplayTagContainer* optionalRelevantTags) const
{
   // Call base class
   if (!Super::CanActivateAbility(handle, actorInfo, sourceTags, targetTags, optionalRelevantTags))
      return false;

   // Currently held tool must be a ranged weapon
   UToolRangedWeaponComponent* rangedWeaponComp = _GetRangedWeaponComponent(actorInfo);
   if (!rangedWeaponComp)
      return false;

   return CanActivateAbility(*rangedWeaponComp);
}

void UToolRangedWeaponAbilityBase::ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData)
{
   Super::ActivateAbility(handle, actorInfo, activationInfo, triggerEventData);

   if (UToolRangedWeaponComponent* rangedWeaponComp = _GetRangedWeaponComponent(actorInfo))
   {
      ActivateAbility(*rangedWeaponComp);
   }
}

void UToolRangedWeaponAbilityBase::EndAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, bool replicateEndAbility, bool wasCancelled)
{
   Super::EndAbility(handle, actorInfo, activationInfo, replicateEndAbility, wasCancelled);

   if (UToolRangedWeaponComponent* rangedWeaponComp = _GetRangedWeaponComponent(actorInfo))
   {
      EndAbility(*rangedWeaponComp);
   }
}

bool UToolRangedWeaponAbilityBase::CommitAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, OUT FGameplayTagContainer* optionalRelevantTags)
{
   if (Super::CommitAbility(handle, actorInfo, activationInfo))
   {
      if (UToolRangedWeaponComponent* rangedWeaponComp = _GetRangedWeaponComponent(actorInfo))
      {
         CommitAbility(*rangedWeaponComp);
      }
      return true;
   }
   return false;
}

bool UToolRangedWeaponAbilityBase::CommitAbilityCost(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, OUT FGameplayTagContainer* optionalRelevantTags)
{
   if (Super::CommitAbilityCost(handle, actorInfo, activationInfo))
   {
      if (UToolRangedWeaponComponent* rangedWeaponComp = _GetRangedWeaponComponent(actorInfo))
      {
         CommitAbilityCost(*rangedWeaponComp);
      }
      return true;
   }
   return false;
}

UToolRangedWeaponComponent* UToolRangedWeaponAbilityBase::GetRangedWeaponComponent() const
{
   if (const FGameplayAbilityActorInfo* actorInfo = GetCurrentActorInfo())
   {
      return _GetRangedWeaponComponent(actorInfo);
   }
   return nullptr;
}

UToolRangedWeaponComponent* UToolRangedWeaponAbilityBase::_GetRangedWeaponComponent(const FGameplayAbilityActorInfo* actorInfo) const
{
   if (auto toolSetSystemInterface = Cast<IToolSetSystemInterface>(actorInfo->AvatarActor.Get()))
   {
      if (TScriptInterface<IToolSetInterface> toolSetInterface = toolSetSystemInterface->GetToolSetInterface())
      {
         return Cast<UToolRangedWeaponComponent>(toolSetInterface->GetCurrentTool());
      }
   }
   return nullptr;
}

