// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/GameplayBehavior/Ambient/GameplayBehavior_Ambient.h"

// tat
#include "AI/GameplayBehavior/Ambient/GameplayBehaviorConfig_Ambient.h"

// ue4
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameplayBehavior_Ambient)

TSubclassOf<UGameplayAbility> UGameplayBehavior_Ambient::GetValidAbilityFromConfig(
   const UGameplayBehaviorConfig_Ambient* configAmbient, const UAbilitySystemComponent& abilitySystemComponent)
{
   FGameplayTagContainer actorTags;
   abilitySystemComponent.GetOwnedGameplayTags(actorTags);
   for (const FAmbientBehaviorDefinition& specificInteraction : configAmbient->SpecificInteractions)
   {
      if(specificInteraction.QueryToMatchForValid.Matches(actorTags))
      {
         return specificInteraction.AbilityToActivate;
      }
   }
   return configAmbient->AbilityToActivate;
}

bool UGameplayBehavior_Ambient::Trigger(AActor& Avatar, const UGameplayBehaviorConfig* Config, AActor* SmartObjectOwner)
{
   const UGameplayBehaviorConfig_Ambient* configAmbient = Cast<const UGameplayBehaviorConfig_Ambient>(Config);
   if(configAmbient == nullptr)
   {
      return false;
   }
   if(UAbilitySystemComponent* abilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(&Avatar))
   {
      _abilityToTrack = GetValidAbilityFromConfig(configAmbient, *abilitySystemComponent);
      if(_abilityToTrack == nullptr)
      {
         return false;
      }
      FGameplayAbilitySpec spec = FGameplayAbilitySpec(configAmbient->AbilityToActivate, 1, INDEX_NONE, SmartObjectOwner);
      abilitySystemComponent->AbilityEndedCallbacks.AddUObject(this, &ThisClass::_OnAbilityCompleted, TWeakObjectPtr<AActor>(&Avatar));
      _trackedAbilitySpecHandle = abilitySystemComponent->GiveAbilityAndActivateOnce(spec);
   }
   return true;
}

void UGameplayBehavior_Ambient::EndBehavior(AActor& Avatar, const bool bInterrupted)
{
   Super::EndBehavior(Avatar, bInterrupted);
   if(bInterrupted)
   {
      if(UAbilitySystemComponent* abilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(&Avatar))
      {
         abilitySystemComponent->CancelAbilityHandle(_trackedAbilitySpecHandle);
      }
   }
}

void UGameplayBehavior_Ambient::_OnAbilityCompleted(UGameplayAbility* gameplayAbility, TWeakObjectPtr<AActor> avatar)
{
   if(avatar.IsValid() == false)
   {
      return;
   }
   if(gameplayAbility && gameplayAbility->IsA(_abilityToTrack))
   {
      if(UAbilitySystemComponent* abilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(avatar.Get()))
      {
         abilitySystemComponent->AbilityEndedCallbacks.RemoveAll(this);
      }
      EndBehavior(*avatar, false);
   }
}
