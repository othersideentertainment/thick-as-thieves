// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Interactables/OSEInteractableToggleSwitchComponent.h"

#include "TATInteractableToggleSwitchComponent.generated.h"

UCLASS(meta = (BlueprintSpawnableComponent))
class TAT_API UTATInteractableToggleSwitchComponent : public UOSEInteractableToggleSwitchComponent
{
   GENERATED_BODY()

   // From IInteractableInterface
   void ShowHighlight_Implementation(bool bShowHighlight);

protected:
   // Optional interact-highlight override tag. 
   // Useful in case owning actor (or another component) is also interactable with its own highlight-meshes that would otherwise conflict
   UPROPERTY(EditDefaultsOnly, Category = "Tags")
   FName InteractHighlightTagOverride = NAME_None;
};
