// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Loot/TATLootActorBase.h"

// tat
#include "Items/TATItemFunctionLibrary.h"
#include "Loot/TATLootStashInteractable.h"
#include "Loot/TATLootInterface.h"

// ue
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLootActorBase)

ATATLootActorBase::ATATLootActorBase()
{
   bReplicates = true;
}

void ATATLootActorBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

bool ATATLootActorBase::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   if (!IsValid(interactingCharacter))
   {
      return false;
   }

   if (!interactingCharacter->Implements<UTATLootInventoryInterface>())
   {
      return false;
   }
   if (!UTATItemFunctionLibrary::CanCharacterPickUpThings(interactingCharacter))
   {
      return false;
   }

   // Deliberately return true without checking for room in inventory first (necessary to display "no room" interact prompt)
   return true;
}
