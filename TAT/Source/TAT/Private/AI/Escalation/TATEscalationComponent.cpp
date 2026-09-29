// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Escalation/TATEscalationComponent.h"

// tat
#include "AI/Escalation/TATEscalationDataAsset.h"
#include "AI/TATAIController.h"
#include "Perception/AIPerceptionSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATEscalationComponent)

void UTATEscalationComponent::BeginPlay()
{
   Super::BeginPlay();
}

void UTATEscalationComponent::SetState(const ETATEscalationState state)
{
   if(state == ETATEscalationState::None)
      return;
   if(state == _currentState)
      return;
   if(_currentState != ETATEscalationState::None)
   {
      ExitState(GetModifierDataForState(_currentState));
   }
   _currentState = state;
   EnterState(GetModifierDataForState(_currentState));
   
   if(UAIPerceptionSystem* aiPerceptionSystem = UAIPerceptionSystem::GetCurrent(GetWorld()))
   {
      AAIController* aiController = CastChecked<AAIController>(GetOwner());
      aiPerceptionSystem->UpdateListener(*aiController->GetPerceptionComponent());
   }
   OnEscalationStateChanged.Broadcast(_currentState);
}

UTATEscalationDataAsset* UTATEscalationComponent::GetModifierDataForState(const ETATEscalationState state)
{
   if(EscalationModifierDataAssets.Contains(state))
      return EscalationModifierDataAssets[state];
   return nullptr;
}

void UTATEscalationComponent::EnterState(const UTATEscalationDataAsset* modifierDataAsset) const
{
   if(modifierDataAsset == nullptr)
      return;
   if(ATATAIController* aiController = Cast<ATATAIController>(GetOwner()))
   {
      modifierDataAsset->ApplyModifiers(aiController);
      modifierDataAsset->ActivateStateGameplayAbility(aiController);
   }
}

void UTATEscalationComponent::ExitState(const UTATEscalationDataAsset* modifierDataAsset) const
{
   if(modifierDataAsset == nullptr)
      return;
   if(ATATAIController* aiController = Cast<ATATAIController>(GetOwner()))
   {
      modifierDataAsset->DeactivateStateGameplayAbility(aiController);
      modifierDataAsset->RevertModifiers(aiController);
   }
}
