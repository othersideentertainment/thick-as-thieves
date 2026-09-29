// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Melee/TATGameplayAbility_MeleeAttack.h"

// tat
#include "Combat/TATCombatComponent.h"
#include "Tools/TATMeleeWeaponToolComponent.h"
#include "Tools/TATToolFunctionLibrary.h"

// ose
#include "Character/OSECharacterBase.h"
#include "Items/ToolSetInterface.h"

// ue
#include "Animation/AnimMontage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayAbility_MeleeAttack)
DEFINE_LOG_CATEGORY_STATIC(LogTATGameplayAbility_MeleeAttack, Log, All);

void UTATGameplayAbility_MeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData)
{
   Super::ActivateAbility(handle, ownerInfo, activationInfo, triggerEventData);
   const bool replicateEndAbility = false;
   const bool wasCancelled = false;

   // Notify combat component of attack start time
   if (!triggerEventData)
   {
      EndAbility(handle, ownerInfo, activationInfo, replicateEndAbility, wasCancelled);
      return;
   }

   const UAnimMontage* animMontage = Cast<UAnimMontage>(triggerEventData->OptionalObject);
   if (!animMontage)
   {
      UE_LOG(LogTATGameplayAbility_MeleeAttack, Error, TEXT("Ability triggered without valid UAnimMontage (expected in OptionalObject)! Ending..."));
      EndAbility(handle, ownerInfo, activationInfo, replicateEndAbility, wasCancelled);
      return;
   }


   // Update state for chainable attack (only players using melee weapon tools can perform these)
   // TODO: remove this conditional once we start using UTATGameplayAbility_DispatchMeleeAttack for AI melee attacks
   const AOSECharacterBase& ownerCharacter = _GetCharacter();
   if (ownerCharacter.IsPlayerControlled())
   {
      // Cache tool usage tag for later retrieval on impact
      _toolUsageTag = UTATToolFunctionLibrary::ExtractToolUsageTypeFromContainer(triggerEventData->InstigatorTags);
      if (_toolUsageTag.IsValid())
      {
         const TScriptInterface<IToolSetInterface> toolsetInterface = ownerCharacter.GetToolSetInterface();
         const UTATMeleeWeaponToolComponent* equippedMeleeWeapon = Cast<UTATMeleeWeaponToolComponent>(toolsetInterface->GetCurrentTool());
         if (!equippedMeleeWeapon)
         {
            UE_LOG(LogTATGameplayAbility_MeleeAttack, Error, TEXT("Ability triggered with toolUsageTag %s, but character does not have a melee weapon tool equipped!"), *_toolUsageTag.ToString());
            EndAbility(handle, ownerInfo, activationInfo, replicateEndAbility, wasCancelled);
            return;
         }
         int32 chainIndex = static_cast<int32>(triggerEventData->EventMagnitude);

         UTATCombatComponent* combatComponent = CastChecked<UTATCombatComponent>(ownerCharacter.GetCombatComponent());
         if (!combatComponent->NotifyStartAttack(_toolUsageTag, equippedMeleeWeapon, animMontage, chainIndex))
         {
            // Canceling the attack might be more than is necessary, but it shouldn't be expected in incidental cases
            UE_LOG(LogTATGameplayAbility_MeleeAttack, Error, TEXT("Notify start failed. Ending..."));
            const bool doReplicateEndAbility = true;
            const bool doCancel = true;
            EndAbility(handle, ownerInfo, activationInfo, doReplicateEndAbility, doCancel);
            return;
         }
      }
   }
}
