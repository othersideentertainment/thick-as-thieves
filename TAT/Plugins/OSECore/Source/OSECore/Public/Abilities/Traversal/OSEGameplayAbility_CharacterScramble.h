// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Traversal/OSEGameplayAbility_Character.h"
#include "OSEGameplayAbility_CharacterScramble.generated.h"


/// Character scramble ability
UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class OSECORE_API UOSEGameplayAbility_CharacterScramble : public UOSEGameplayAbility_Character
{
   GENERATED_BODY()

public:

   UOSEGameplayAbility_CharacterScramble();

protected:

   virtual bool CanActivateAbility(ACharacter* character) const override;
   virtual void ActivateAbility(ACharacter* character) override;
   virtual void CancelAbility(ACharacter* character) override;
};
