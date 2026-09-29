// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Traversal/OSEGameplayAbility_Character.h"
#include "OSEGameplayAbility_CharacterCrouch.generated.h"


/// Character crouch ability
UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class OSECORE_API UOSEGameplayAbility_CharacterCrouch : public UOSEGameplayAbility_Character
{
   GENERATED_BODY()

protected:

   virtual bool CanActivateAbility(ACharacter* character) const override;
   virtual void ActivateAbility(ACharacter* character) override;
   virtual void CancelAbility(ACharacter* character) override;

   /// True if the character uncrouches when the ability is cancelled
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
   bool UnCrouchWhenCancelled = false;
};
