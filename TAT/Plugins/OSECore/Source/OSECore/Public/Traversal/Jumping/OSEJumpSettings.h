// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "OSEJumpSettings.generated.h"


/// External forces applied to a character while jumping
USTRUCT(BlueprintType)
struct OSECORE_API FOSEJumpPhysics
{
   GENERATED_BODY()

public:

   /// The local space velocity to SET on the character.
   /// Components with a zero value are not applied.
   /// \see UCharacterMovementComponent::JumpZVelocity
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Jumping / Falling")
   FVector Velocity = FVector::ZeroVector;

   /// The local space velocity to ADD to the character's velocity.
   /// \see UCharacterMovementComponent::AddImpulse()
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Jumping / Falling")
   FVector Impulse = FVector::ZeroVector;

   // The local space acceleration to add to the character's velocity over time.
   /// \see UCharacterMovementComponent::AddForce()
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Jumping / Falling")
   FVector Acceleration = FVector::ZeroVector;
};


/// Character jump configuration. This specifies the values to use for one specific jump.
/// This allows multiple jumps to use different settings.
/// \see ACharacter::JumpMaxCount
USTRUCT(BlueprintType)
struct OSECORE_API FOSEJumpSettings
{
   GENERATED_BODY()

public:

   /// The jump physics to apply when first starting to jump.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Jumping / Falling")
   FOSEJumpPhysics InitialPhysics;

   /// The jump physics to apply while the jump key is held.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Jumping / Falling")
   FOSEJumpPhysics HoldPhysics;

   /// Limits the max hold time to this value if > 0. Ignored otherwise.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Jumping / Falling", meta = (ClampMin = "0", UIMin = "0"))
   float MaxHoldTime = 0.0f;

   /// When true, hold physics will only be applied after the apex of the jump is reached. This does not apply to velocity, which is always applied while holding. Hold time remains unaffected.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Jumping / Falling")
   bool DeferHoldUntilApex = false;
};


//---------------------------------------------------------------------------------------
/// Defines the behavior when attempting a jump while crouched.
//---------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FOSECrouchJumpSettings
{
   GENERATED_BODY()

public:

   /// When enabled, jumping is allowed when crouching
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Jumping / Falling")
   bool AllowJumpWhileCrouched = false;

   /// Defines if a crouch jump causes the character to uncrouch or not
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Jumping / Falling", meta = (editcondition = "AllowJumpWhileCrouched"))
   bool UncrouchWhenJumping = false;
};
