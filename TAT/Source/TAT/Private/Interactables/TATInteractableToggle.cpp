// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/TATInteractableToggle.h"

// tat
#include "AI/Target/TATTargetingGroups.h"
#include "Breakables/TATBreakableActorImpl.h"
#include "Environment/TATInhibitorSubsystem.h"
#include "Interactables/TATInteractHighlightUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATInteractableToggle)

ATATInteractableToggle::ATATInteractableToggle()
{
   _abilitySystemComponent = CreateAbilitySystemForBreakables(this);
   _breakableComponent = CreateDefaultSubobject<UTATBreakableComponent>(TEXT("BreakableComponent"));
   _breakableComponent->SetIsBreakableByDefault(false); //< Keep it opt-in for now, given surface area
   bReplicateUsingRegisteredSubObjectList = true;
}

void ATATInteractableToggle::PostInitializeComponents()
{
   Super::PostInitializeComponents();

   _interactionGates.Initialize(this);
}

bool ATATInteractableToggle::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   if(!Super::IsInteractable_Implementation(interactingCharacter))
   {
      return false;
   }

   if (_interactionGates.IsInteractionBlockedWithoutMessage())
   {
      return false;
   }

   if (IsInhibitable)
   {
      UTATInhibitorSubsystem* inhibitorSubsystem = GetWorld()->GetSubsystem<UTATInhibitorSubsystem>();
      if (inhibitorSubsystem && inhibitorSubsystem->IsActorCurrentlyInhibited(this))
      {
         return false;
      }
   }

   return _interactableWhenBroken || !_breakableComponent->IsBroken();
}

void ATATInteractableToggle::ShowHighlight_Implementation(bool bShowHighlight)
{
   UTATInteractHighlightUtils::HighlightInteractMeshes(this, bShowHighlight);
}

void ATATInteractableToggle::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   if(!_TryPriorityInteractPrompt(interactingCharacter, prompt))
   {
      _AddNormalInteractPrompt(interactingCharacter, prompt);
   }
}

FInteractStartResult ATATInteractableToggle::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   FInteractStartResult result;
   if(_TryPriorityStartInteract(interactingCharacter, result))
   {
      return result;
   }

   return _NormalStartInteract(interactingCharacter);
}

FGameplayTag ATATInteractableToggle::GetUtilityAITargetingGroup() const
{
   if(_breakableComponent && _breakableComponent->IsBroken())
   {
      return TAG_AI_TargetingGroup_SmartObject_BrokenObject;
   }
   return FGameplayTag::EmptyTag;
}

void ATATInteractableToggle::GetActorTraitsForVoiceLines(FGameplayTagContainer& tagContainer) const
{
   if(_breakableComponent && _breakableComponent->IsBroken())
   {
      tagContainer.AddTag(_tagForVoiceLinesWhenBroken);
   }
}

bool ATATInteractableToggle::CanBeInhibitedBy_Implementation(FGameplayTag inhibitorType) const
{
   if (!IsInhibitable)
   {
      return false;
   }
   return true;
}

FTATInhibitorPlacementInfo ATATInteractableToggle::GetInhibitorPlacementInfo_Implementation() const
{
   // Child classes will almost certainly want to override this, but we can provide a simple baseline implementation here
   return FTATInhibitorPlacementInfo::Make(GetActorLocation(), GetActorRotation());
}

void ATATInteractableToggle::OnInhibitorActivated_Implementation(ATATInhibitorActor* inhibitorActor, APawn* instigator, int32 newInhibitorCount)
{
   if (!IsInhibitable)
   {
      return;
   }
   if (State.bIsOn)
   {
      SetOn(false);
   }
}

void ATATInteractableToggle::OnInhibitorDeactivated_Implementation(ATATInhibitorActor* inhibitorActor, int32 newInhibitorCount, bool allInhibitorsRemoved)
{
}

BREAKABLE_ACTOR_IMPLS(ATATInteractableToggle, _abilitySystemComponent, _breakableComponent)

bool ATATInteractableToggle::_TryPriorityInteractPrompt(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   return _interactionGates.TryAddToPrompt(prompt);
}

void ATATInteractableToggle::_AddNormalInteractPrompt(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   Super::GetInteractPrompt_Implementation(interactingCharacter, prompt);
}

bool ATATInteractableToggle::_TryPriorityStartInteract(ACharacter* interactingCharacter, FInteractStartResult& outResult)
{
   if(_interactionGates.IsInteractionBlocked())
   {
      return true;
   }
   
   return false;
}

FInteractStartResult ATATInteractableToggle::_NormalStartInteract(ACharacter* interactingCharacter)
{
   return Super::StartInteract_Implementation(interactingCharacter);
}
