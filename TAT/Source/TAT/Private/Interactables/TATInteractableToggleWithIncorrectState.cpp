// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/TATInteractableToggleWithIncorrectState.h"

// tat
#include "AI/SmartObjects/TATActionNodeComponent.h"
#include "AI/SmartObjects/TATActionNodeComponent_IncorrectObjectState.h"
#include "AI/SmartObjects/TATSmartObjectObjectTags.h"
#include "AI/Target/TATTargetingGroups.h"
#include "Breakables/TATBreakableComponent.h"

// ose
#include "AI/OSEAIFunctionLibrary.h"

// ue
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATInteractableToggleWithIncorrectState)

ATATInteractableToggleWithIncorrectState::ATATInteractableToggleWithIncorrectState()
{
   RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
   RootComponent->Mobility = EComponentMobility::Static;

   IncorrectStateActionNodeComponent = CreateDefaultSubobject<UTATActionNodeComponent_IncorrectObjectState>(TEXT("IncorrectStateActionNode"));
   IncorrectStateActionNodeComponent->SetupAttachment(RootComponent);
   IncorrectStateActionNodeComponent->Mobility = EComponentMobility::Static;
}

void ATATInteractableToggleWithIncorrectState::PostInitializeComponents()
{
   Super::PostInitializeComponents();

   check(IncorrectStateActionNodeComponent != nullptr);
   if (HasAuthority())
   {
      // Incorrect state component will pay attention to whether this actor is broken or not.
      // If broken, the object will be marked as not in an incorrect state as it's a state 
      // the AI can't do anything about
      IncorrectStateActionNodeComponent->AssignBreakableComponent(_breakableComponent);

      const int32 currentState = IsOn() ? 1 : 0;
      IncorrectStateActionNodeComponent->SetInitialState(currentState, currentState);
   }
}

FGameplayTag ATATInteractableToggleWithIncorrectState::GetUtilityAITargetingGroup() const
{
   if (_breakableComponent && _breakableComponent->IsBroken())
   {
      return TAG_AI_TargetingGroup_SmartObject_BrokenObject;
   }
   return TAG_AI_TargetingGroup_SmartObject_IncorrectState;
}

bool ATATInteractableToggleWithIncorrectState::AuthorityIsObjectInCorrectState_Implementation(bool allowIgnoringOfState) const
{
   check(IncorrectStateActionNodeComponent != nullptr);
   return IncorrectStateActionNodeComponent->IsStateCorrect(allowIgnoringOfState);
}

FGameplayTagCountContainer& ATATInteractableToggleWithIncorrectState::GetGameplayTagCountContainer()
{
   check(IncorrectStateActionNodeComponent != nullptr);
   return IncorrectStateActionNodeComponent->GetIncorrectStateTags();
}

UTATSmartObjectComponent* ATATInteractableToggleWithIncorrectState::GetSmartObjectComponent() const
{
   return IncorrectStateActionNodeComponent;
}

void ATATInteractableToggleWithIncorrectState::_OnStateChanged(bool bIsOn, bool bWasRecent)
{
   Super::_OnStateChanged(bIsOn, bWasRecent);

   check(IncorrectStateActionNodeComponent != nullptr);
   const int32 currentState = IsOn() ? 1 : 0;
   IncorrectStateActionNodeComponent->SetCurrentState(currentState);
}
