// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Traps/Old/DisableTrapInteractable.h"

// tat
#include "Interactables/TATInteractHighlightUtils.h"
#include "Traps/Old/TrapTriggerBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DisableTrapInteractable)

// Sets default values
ADisableTrapInteractable_Old::ADisableTrapInteractable_Old()
{
    PrimaryActorTick.bCanEverTick = false;

   _trapDisableDuration = 10;
}

bool ADisableTrapInteractable_Old::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   return _trapToDisable && _trapToDisable->IsCurrentlyArmed();
}

void ADisableTrapInteractable_Old::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   prompt.PressAction = _promptText;
}

FInteractStartResult ADisableTrapInteractable_Old::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   if (_trapToDisable && HasAuthority())
   {
      _trapToDisable->AuthorityDisableTemporarily(_trapDisableDuration);
   }

   return FInteractStartResult();
}

void ADisableTrapInteractable_Old::ShowHighlight_Implementation(bool bShowHighlight)
{
   UTATInteractHighlightUtils::HighlightInteractMeshes(this, bShowHighlight);
}

