// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Interactables/InteractionSphereTrigger.h"

// ose
#include "Interactables/OSEInteractionHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractionSphereTrigger)

UInteractionSphereTrigger::UInteractionSphereTrigger()
{   
   InitSphereRadius(30.0f);
   SetCollisionProfileName(FName(TEXT("InteractOnly")));
   ComponentTags.Add(UOSEInteractionHelpers::kInteractNoHighlightTag);

   SetCanEverAffectNavigation(false);
}

