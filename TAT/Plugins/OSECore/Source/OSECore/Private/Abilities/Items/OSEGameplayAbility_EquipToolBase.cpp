// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Items/OSEGameplayAbility_EquipToolBase.h"

// ose
#include "Character/OSECharacterBase.h"
#include "Items/ToolComponent.h"
#include "Items/ToolSetInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameplayAbility_EquipToolBase)

// ue4

UOSEGameplayAbility_EquipToolBase::UOSEGameplayAbility_EquipToolBase()
{
   // local only to reduce GA instances
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

bool UOSEGameplayAbility_EquipToolBase::CanActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayTagContainer* sourceTags, const FGameplayTagContainer* targetTags, FGameplayTagContainer* optionalRelevantTags) const
{
   const bool canActivateBase = Super::CanActivateAbility(handle, actorInfo, sourceTags, targetTags, optionalRelevantTags);
   bool canActivate = false;
   if (canActivateBase)
   {
      if (AOSECharacterBase* character = _GetCharacter(actorInfo))
      {
         if (TSubclassOf<UToolComponent> toolClass = _GetToolClassToEquip(actorInfo))
         {
            canActivate = character->GetToolSetInterface()->HasToolClass(toolClass);
         }
      }
   }
   return canActivateBase && canActivate;
}

void UOSEGameplayAbility_EquipToolBase::ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData)
{
   Super::ActivateAbility(handle, actorInfo, activationInfo, triggerEventData);

   if (AOSECharacterBase* character = _GetCharacter(actorInfo))
   {
      if (TSubclassOf<UToolComponent> toolClass = _GetToolClassToEquip(actorInfo))
      {
         if (CommitAbility(handle, actorInfo, activationInfo))
         {
            if (character->GetToolSetInterface()->EquipToolByClass(toolClass))
            {
               _OnToolClassEquipped(actorInfo, toolClass);
            }
         }
      }
   }

   // end ability immediately
   bool replicateEndAbility = true;
   bool wasCancelled = false;
   EndAbility(handle, actorInfo, activationInfo, replicateEndAbility, wasCancelled);
}

void UOSEGameplayAbility_EquipToolBase::EndAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, bool replicateEndAbility, bool wasCancelled)
{
   Super::EndAbility(handle, actorInfo, activationInfo, replicateEndAbility, wasCancelled);
}

TSubclassOf<UToolComponent> UOSEGameplayAbility_EquipToolBase::_GetToolClassToEquip(const FGameplayAbilityActorInfo* actorInfo) const
{
   return nullptr;
}

AOSECharacterBase* UOSEGameplayAbility_EquipToolBase::_GetCharacter(const FGameplayAbilityActorInfo* actorInfo) const
{
   return CastChecked<AOSECharacterBase>(actorInfo->AvatarActor.Get());
}

