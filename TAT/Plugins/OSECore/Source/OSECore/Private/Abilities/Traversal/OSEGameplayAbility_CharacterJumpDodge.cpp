// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Traversal/OSEGameplayAbility_CharacterJumpDodge.h"

// ose
#include "Abilities/OSEAbilityInputBinds.h"
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Character/OSECharacterMovement.h"

// ue4
#include "AbilitySystemGlobals.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputLibrary.h"
#include "Kismet/KismetMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameplayAbility_CharacterJumpDodge)

namespace CharacterJumpDodgeUtl
{
   UOSECharacterMovement* GetCharacterMovement(const ACharacter* character)
   {
      check(character);
      return Cast<UOSECharacterMovement>(character->GetMovementComponent());
   }
}

UOSEGameplayAbility_CharacterJumpDodge::UOSEGameplayAbility_CharacterJumpDodge()
   : Super()
   , ThumbstickUpDirection(0.0f, 1.0f)
{
   // local-only ability; this ability triggers an actual dodge or jump ability depending on input state
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
   InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
   
   // requires a player controller to grab an input axis
   ActivationRequiresPlayerController = true;
}

bool UOSEGameplayAbility_CharacterJumpDodge::CanActivateAbility(ACharacter* character) const
{
   if (UOSECharacterMovement* movementComp = CharacterJumpDodgeUtl::GetCharacterMovement(character))
   {
      return character->CanJump() || movementComp->CanDodgeInCurrentState();
   }
   return false;
}

void UOSEGameplayAbility_CharacterJumpDodge::ActivateAbility(ACharacter* character)
{
   _ResetState();

   UOSECharacterMovement* movementComp = CharacterJumpDodgeUtl::GetCharacterMovement(character);
   ensure(movementComp); // requires a movement comp, we should always have it anyway...
   if (!movementComp)
      return;

   UOSEAbilitySystemComponent* asc = CastChecked<UOSEAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
   
   FVector2D stickDirection = FVector2D::ZeroVector;
   if (MoveInputAction)
   {
      FInputActionValue stickDirectionVal = UEnhancedInputLibrary::GetBoundActionValue(character, MoveInputAction);
      stickDirection = FVector2D(stickDirectionVal[0], stickDirectionVal[1]);
   }

   bool shouldJump = false;
   bool shouldDodge = false;
   if (stickDirection.IsNearlyZero())
   {
      // if we have no movement it's a jump and we can execute it here locally
      shouldJump = true;
   }
   else
   {
      switch(Config)
      {
      case EJumpDodgeConfiguration::ForwardJumpSideBackDodge:
         {
            // otherwise, a forward movement is a jump, and a side/back movement is a dodge
            const float multiplier = (stickDirection.X >= 0.0f) ? 1.0f : -1.0f;
            const float dot = stickDirection | ThumbstickUpDirection;
            float rotDeg = (UKismetMathLibrary::DegAcos(dot) * multiplier);
            const bool isJumpThumbstickAngle = FMath::Abs(rotDeg) <= ThumbstickJumpAngle;
            shouldJump = isJumpThumbstickAngle;
            shouldDodge = !shouldJump;
         }
         break;
      case EJumpDodgeConfiguration::ForwardSideJumpBackDodge:
         {
            // otherwise, a backward movement is a dodge, and a side/forward movement is a jump
            const float multiplier = (stickDirection.X >= 0.0f) ? 1.0f : -1.0f;
            const float dot = stickDirection | -ThumbstickUpDirection;
            float rotDeg = (UKismetMathLibrary::DegAcos(dot) * multiplier);
            const bool isDodgeThumbstickAngle = FMath::Abs(rotDeg) <= ThumbstickJumpAngle;
            shouldDodge = isDodgeThumbstickAngle;
            shouldJump = !shouldDodge;
         }
         break;
      }
   }

   if (shouldJump && character->CanJump())
   {
      _jumped = true;
      character->Jump();
   }

   if (shouldDodge && movementComp->CanDodgeInCurrentState())
   {
      // dodge needs to have it's own ability executed so that it is locally predicted but runs on the server too
      _dodged = true;
      asc->TryActivateAbilityByInputID(EAbilityInputType::AbilityMovementDodge);
   }
}

void UOSEGameplayAbility_CharacterJumpDodge::CancelAbility(ACharacter* character)
{
   if (_jumped)
   {
      character->StopJumping();
   }

   if (_dodged)
   {
      // nada?
   }

   _ResetState();
}

void UOSEGameplayAbility_CharacterJumpDodge::_ResetState()
{
   _jumped = false;
   _dodged = false;
}

