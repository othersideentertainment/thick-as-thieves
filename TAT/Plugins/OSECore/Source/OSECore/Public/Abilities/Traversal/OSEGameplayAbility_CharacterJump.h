// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Traversal/OSEGameplayAbility_Character.h"
#include "OSEGameplayAbility_CharacterJump.generated.h"


/// Character jump ability
UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class OSECORE_API UOSEGameplayAbility_CharacterJump : public UOSEGameplayAbility_Character
{
   GENERATED_BODY()

public:

   UOSEGameplayAbility_CharacterJump();

protected:

   virtual bool CanActivateAbility(ACharacter* character) const override;
   virtual void ActivateAbility(ACharacter* character) override;
   virtual void CancelAbility(ACharacter* character) override;
};
