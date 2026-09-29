// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/Escalation/Abilities/TATGameplayAbility_Escalation.h"

// tat
#include "AI/TATAIController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayAbility_Escalation)

void UTATGameplayAbility_Escalation::SetEscalationState(ETATEscalationState newState)
{
   const ATATAIController* tatAIController = GetTATAIController();
   if(tatAIController == nullptr)
      return;
   UTATEscalationComponent* escalationComponent = tatAIController->GetTATEscalationComponent();
   if(escalationComponent == nullptr)
      return;
   escalationComponent->SetState(newState);   
}

UTATAlertnessComponent* UTATGameplayAbility_Escalation::GetTATAlertnessComponentFromActorInfo() const
{
   const ATATAIController* tatAIController = GetTATAIController();
   if(tatAIController == nullptr)
      return nullptr;
   return Cast<UTATAlertnessComponent>(tatAIController->GetAlertnessComponent());
}

ATATAIController* UTATGameplayAbility_Escalation::GetTATAIController() const
{
   const APawn* pawn = Cast<APawn>(GetOwningActorFromActorInfo());
   if(pawn == nullptr)
      return nullptr;
   return pawn->GetController<ATATAIController>();
}
