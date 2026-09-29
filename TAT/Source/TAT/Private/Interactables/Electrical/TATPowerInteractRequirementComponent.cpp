// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Interactables/Electrical/TATPowerInteractRequirementComponent.h"

// tat
#include "Interactables/TATSwingingDoor.h"
#include "Interactables/Electrical/TATElectricalDeviceComponent.h"
#include "Interactables/Electrical/TATPowerSource.h"

// ose
#include "Interactables/OSEInteractableToggle.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPowerInteractRequirementComponent)

UTATPowerInteractRequirementComponent::UTATPowerInteractRequirementComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
}

void UTATPowerInteractRequirementComponent::BeginPlay()
{
   Super::BeginPlay();

   if(!CanEverBlockInteraction())
   {
      return;
   }

   if(const UTATElectricalDeviceComponent* electricalDevice = GetOwner()->GetComponentByClass<UTATElectricalDeviceComponent>())
   {
      _powerSource = electricalDevice->GetPowerSource();
   }

   if(_powerSource && GetOwner()->HasAuthority() && _closeWhenUnusable)
   {
      _powerSource->OnPowerStateChanged.AddUObject(this, &ThisClass::_OnAuthorityPoweredChanged);
   }
}

bool UTATPowerInteractRequirementComponent::IsInteractionBlocked() const
{
   return _powerSource && _powerSource->IsPowered() == (_mode == ETATPowerInteractRequireMode::RequireUnpowered);
}

void UTATPowerInteractRequirementComponent::AddToPrompt(FInteractPrompt& prompt) const
{
   prompt.ErrorMessage = _unusableMessage;
}

void UTATPowerInteractRequirementComponent::_OnAuthorityPoweredChanged(bool hasPower)
{
   if(IsInteractionBlocked() && _closeWhenUnusable)
   {
      // a little nose-holding here with the cast
      if(AOSESyncedToggle* toggle = GetOwner<AOSESyncedToggle>())
      {
         toggle->TurnOff();
      }
      else if(ATATSwingingDoor* door = GetOwner<ATATSwingingDoor>())
      {
         door->SetClosedState(true);
      }
   }
}
