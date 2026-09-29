// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Interactables/TATInteractHighlightUtils.h"

// tat
#include "Graphics/TATHighlightStateMgrComponent.h"

// ue4
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATInteractHighlightUtils)

const FName UTATInteractHighlightUtils::InteractHighlightTag_NAME("Interact");

void UTATInteractHighlightUtils::HighlightInteractMeshes(AActor* actor, bool isHighlighted)
{
   if (UTATHighlightStateMgrComponent* mgr = UTATHighlightStateMgrComponent::FindOrAdd(actor))
   {
      static const FName kInteractSystemName = TEXT("Interact");
      mgr->RequestHighlightChange(kInteractSystemName, isHighlighted);
   }
}

