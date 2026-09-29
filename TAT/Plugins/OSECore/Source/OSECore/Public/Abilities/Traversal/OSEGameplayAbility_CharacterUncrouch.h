// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Traversal/OSEGameplayAbility_Character.h"
#include "OSEGameplayAbility_CharacterUncrouch.generated.h"


/// Character uncrouch ability
UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class OSECORE_API UOSEGameplayAbility_CharacterUncrouch : public UOSEGameplayAbility_Character
{
   GENERATED_BODY()

protected:

   virtual bool CanActivateAbility(ACharacter* character) const override;
   virtual void ActivateAbility(ACharacter* character) override;
};
