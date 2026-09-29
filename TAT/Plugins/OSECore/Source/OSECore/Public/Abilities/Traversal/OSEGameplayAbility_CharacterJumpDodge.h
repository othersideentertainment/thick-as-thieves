// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Traversal/OSEGameplayAbility_Character.h"
#include "OSEGameplayAbility_CharacterJumpDodge.generated.h"

class UInputAction;

UENUM(BlueprintType)
enum class EJumpDodgeConfiguration : uint8
{
   ForwardJumpSideBackDodge,
   ForwardSideJumpBackDodge,
};

/// Character jump + dodge ability
UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class OSECORE_API UOSEGameplayAbility_CharacterJumpDodge : public UOSEGameplayAbility_Character
{
   GENERATED_BODY()

public:

   UOSEGameplayAbility_CharacterJumpDodge();

   // Which direction do we consider up?
   UPROPERTY(EditDefaultsOnly, Category = "Jump / Dodge Settings")
   FVector2D ThumbstickUpDirection;
   
   // How far to the left/right of this direction are we triggering for?
   UPROPERTY(EditDefaultsOnly, Category = "Jump / Dodge Settings")
   float ThumbstickJumpAngle = 65.0f;

   // The move action we use to determine thumbstick direction
   UPROPERTY(EditDefaultsOnly, Category = "Jump / Dodge Settings")
   UInputAction* MoveInputAction = nullptr;

   // Alternate configuration -- use the same angles but to determine alternate axis
   UPROPERTY(EditDefaultsOnly, Category = "Jump / Dodge Settings")
   EJumpDodgeConfiguration Config = EJumpDodgeConfiguration::ForwardSideJumpBackDodge;

protected:
   // from UOSEGameplayAbility_Character
   virtual bool CanActivateAbility(ACharacter* character) const override;
   virtual void ActivateAbility(ACharacter* character) override;
   virtual void CancelAbility(ACharacter* character) override;

private:
   void _ResetState();

private:
   bool _jumped = false;
   bool _dodged = false;
};
