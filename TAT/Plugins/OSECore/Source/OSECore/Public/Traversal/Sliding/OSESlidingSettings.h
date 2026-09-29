// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "OSESlidingSettings.generated.h"


/// Sliding configuration settings
USTRUCT(BlueprintType)
struct OSECORE_API FOSESlidingSettings
{
   GENERATED_BODY()

public:

   /// Deceleration when sliding and not applying acceleration. This is a constant opposing force that directly lowers velocity by a constant value.
   /// \see UCharacterMovementComponent::BrakingDecelerationWalking
   UPROPERTY(Category = "Character Movement: Sliding", EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", UIMin = "0"))
   float BrakingDeceleration = 0.0f;

   /// When braking while sliding, this property modifies the ground friction, applying an opposing force that scales with current velocity.
   /// \see UCharacterMovementComponent::GroundFriction
   UPROPERTY(Category = "Character Movement: Sliding", EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", UIMin = "0"))
   float FrictionFactor = 0.0f;

   /// The default behavior is to ignore turn-in-place if enabled, and allow free look while sliding. Enabling this setting will cause sliding to
   /// respect turn-in-place and behave as other ground based movement does when applying delta rotation to the pawn.
   /// \see UCharacterMovementComponent::GetDeltaRotation()
   UPROPERTY(Category = "Character Movement: Sliding", EditAnywhere, BlueprintReadOnly)
   bool AllowTurnInPlace = false;
};
