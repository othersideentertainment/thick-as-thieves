// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/Escalation/Modifiers/TATEscalationModifier.h"

// ue
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

// tat
#include "AI/TATAIController.h"
#include "AI/Utility/TATUtilityAIBehaviorComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATEscalationModifier)

UAbilitySystemComponent* GetAbilitySystemComponent(const ATATAIController* aiController)
{
   check(aiController);
   return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(aiController->GetPawn());
}

void UTATEscalationModifier_GameplayEffect::Apply(ATATAIController* aiController) const
{
   Super::Apply(aiController);
   if(UAbilitySystemComponent* abilitySystemComponent = GetAbilitySystemComponent(aiController))
   {
      for (const TSubclassOf<UGameplayEffect>& effectToApply : EffectsToApply)
      {
         if(effectToApply == nullptr)
            continue;
         const UGameplayEffect* gameplayEffect = effectToApply->GetDefaultObject<UGameplayEffect>();
         if(gameplayEffect == nullptr)
            continue;
         abilitySystemComponent->ApplyGameplayEffectToSelf(gameplayEffect, 0.f, abilitySystemComponent->MakeEffectContext());
      }
   }
}

void UTATEscalationModifier_GameplayEffect::Revert(ATATAIController* aiController) const
{
   Super::Revert(aiController);
   if(UAbilitySystemComponent* abilitySystemComponent = GetAbilitySystemComponent(aiController))
   {
      for (const TSubclassOf<UGameplayEffect>& effectToApply : EffectsToApply)
      {
         abilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(effectToApply, abilitySystemComponent, 1);
      }
   }
}

void UTATEscalationModifier_Behaviors::Apply(ATATAIController* aiController) const
{
   Super::Apply(aiController);
   if(UTATUtilityAIBehaviorComponent* tatUtilityAIBehaviorComponent = Cast<UTATUtilityAIBehaviorComponent>(aiController->GetUtilityAIBehaviorComponent()))
   {
      tatUtilityAIBehaviorComponent->AddInjectedBehaviors(BehaviorInjectionTag, BehaviorsToAdd);
   }
}

void UTATEscalationModifier_Behaviors::Revert(ATATAIController* aiController) const
{
   Super::Revert(aiController);
   if(UTATUtilityAIBehaviorComponent* tatUtilityAIBehaviorComponent = Cast<UTATUtilityAIBehaviorComponent>(aiController->GetUtilityAIBehaviorComponent()))
   {
      tatUtilityAIBehaviorComponent->RemoveInjectedBehaviors(BehaviorInjectionTag);
   }
}
