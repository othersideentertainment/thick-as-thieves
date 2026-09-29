// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/Traversal/OSEGameplayAbility_Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameplayAbility_Character)


UOSEGameplayAbility_Character::UOSEGameplayAbility_Character()
   : Super()
{
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
   InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
   ActivationRequiresMoveInput = true;
}

ACharacter* UOSEGameplayAbility_Character::GetCharacter(const FGameplayAbilityActorInfo* actorInfo) const
{
   return Cast<ACharacter>(actorInfo->AvatarActor.Get());
}

void UOSEGameplayAbility_Character::ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData)
{
   // NOTE: explicitly not calling Super, because that is sort of what they expect you to do for c++ abilities

   if (CommitAbility(handle, actorInfo, activationInfo))
   {
      if (ACharacter* character = GetCharacter(actorInfo))
      {
         ActivateAbility(character);

         // N.B. K2_ActivateAbility() would normally be called by UGameplayAbility::ActivateAbility(), so instead we have a custom event
         BP_ActivateOnCharacter(character);
      }
   }
}

void UOSEGameplayAbility_Character::InputReleased(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo)
{
   if (actorInfo == nullptr || actorInfo->AvatarActor == nullptr)
      return;

   CancelAbility(handle, actorInfo, activationInfo, true);
}

bool UOSEGameplayAbility_Character::CanActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayTagContainer* sourceTags, const FGameplayTagContainer* targetTags, OUT FGameplayTagContainer* optionalRelevantTags) const
{
   if (!Super::CanActivateAbility(handle, actorInfo, sourceTags, targetTags, optionalRelevantTags))
      return false;

   if (ACharacter* character = GetCharacter(actorInfo))
   {
      return CanActivateAbility(character);
   }
   else
   {
      // Character is required
      return false;
   }
}

void UOSEGameplayAbility_Character::EndAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, bool replicateEndAbility, bool wasCancelled)
{
   if (IsEndAbilityValid(handle, actorInfo))
   {
      if (ScopeLockCount > 0)
      {
         WaitingToExecute.Add(FPostLockDelegate::CreateUObject(this, &UOSEGameplayAbility_Character::EndAbility, handle, actorInfo, activationInfo, replicateEndAbility, wasCancelled));
         return;
      }

      if (ACharacter* character = GetCharacter(actorInfo))
      {
         CancelAbility(character);
      }

      Super::EndAbility(handle, actorInfo, activationInfo, replicateEndAbility, wasCancelled);
   }
}

