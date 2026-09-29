// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Character/TATAstralProjectionCharacter.h"

// ue5
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAstralProjectionCharacter)


ATATAstralProjectionCharacter::ATATAstralProjectionCharacter()
{
}

void ATATAstralProjectionCharacter::BeginPlay()
{
   Super::BeginPlay();

   OnCharacterReady.AddUniqueDynamic(this, &ATATAstralProjectionCharacter::_OnCharacterReady);
}

void ATATAstralProjectionCharacter::EndPlay(const EEndPlayReason::Type reason)
{
   OnCharacterReady.RemoveDynamic(this, &ATATAstralProjectionCharacter::_OnCharacterReady);
   Super::EndPlay(reason);
}

void ATATAstralProjectionCharacter::_OnCharacterReady(AOSECharacterBase* character)
{
   if (IsLocallyControlled() && _readyAbilityTag.IsValid())
   {
      if (UAbilitySystemComponent* asc = GetAbilitySystemComponent())
      {
         asc->TryActivateAbilitiesByTag(_readyAbilityTag.GetSingleTagContainer());
      }
   }
}

