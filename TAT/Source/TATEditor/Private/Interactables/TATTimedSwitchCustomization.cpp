// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Interactables/TATTimedSwitchCustomization.h"

// ose
#include "Interactables/OSEInteractableToggle.h"

// ue
#include "DetailLayoutBuilder.h"

void FTATTimedSwitchDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& detailBuilder)
{
   // Hide properties that are not relevant to timed switches
   detailBuilder.HideProperty(FName("State"), AOSESyncedToggle::StaticClass());
   detailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(AOSESyncedToggle, AllowedToggleTransition), AOSESyncedToggle::StaticClass());
   detailBuilder.HideProperty(FName("TurnOffPrompt"), AOSEInteractableToggle::StaticClass());
}
