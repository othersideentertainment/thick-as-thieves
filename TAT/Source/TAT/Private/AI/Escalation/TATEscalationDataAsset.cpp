// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/Escalation/TATEscalationDataAsset.h"

// TAT
#include "AI/TATAIController.h"

// UE
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATEscalationDataAsset)

void UTATEscalationDataAsset::ApplyModifiers(ATATAIController* aiController) const
{
   for (TObjectPtr<UTATEscalationModifier> modifier : EscalationModifiers)
   {
      if(modifier == nullptr)
         continue;;
      modifier->Apply(aiController);
   }
}

void UTATEscalationDataAsset::RevertModifiers(ATATAIController* aiController) const
{
   for (TObjectPtr<UTATEscalationModifier> modifier : EscalationModifiers)
   {
      if(modifier == nullptr)
         continue;;
      modifier->Revert(aiController);
   }
}

void UTATEscalationDataAsset::ActivateStateGameplayAbility(const ATATAIController* aiController) const
{
   check(aiController);
   if(StateAbility == nullptr)
      return;
   if(UAbilitySystemComponent* abilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(aiController->GetPawn()))
   {
      FGameplayAbilitySpec abilitySpec = abilitySystemComponent->BuildAbilitySpecFromClass(StateAbility);
      abilitySystemComponent->GiveAbilityAndActivateOnce(abilitySpec, nullptr);
   }
}

void UTATEscalationDataAsset::DeactivateStateGameplayAbility(const ATATAIController* aiController) const
{
   check(aiController);
   if(StateAbility == nullptr)
      return;
   if(UAbilitySystemComponent* abilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(aiController->GetPawn()))
   {
      abilitySystemComponent->CancelAbility(StateAbility->GetDefaultObject<UGameplayAbility>());
   }
}
