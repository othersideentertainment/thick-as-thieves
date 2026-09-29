// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/TATInteractableToggleSwitchComponent.h"

// tat
#include "Graphics/TATHighlightStateMgrComponent.h"
#include "Interactables/TATInteractHighlightUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATInteractableToggleSwitchComponent)

void UTATInteractableToggleSwitchComponent::ShowHighlight_Implementation(bool bShowHighlight)
{
   // Search for optional highlight-override tag, if assigned
   if (!InteractHighlightTagOverride.IsNone())
   {
      UTATHighlightStateMgrComponent::HighlightMeshesWithTag(GetOwner(), InteractHighlightTagOverride, bShowHighlight);
   }
   else
   {
      UTATInteractHighlightUtils::HighlightInteractMeshes(GetOwner(), bShowHighlight);
   }
}
