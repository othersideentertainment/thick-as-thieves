// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "Engine/EngineTypes.h"

#include "OSECharacterMovementTypes.generated.h"


//---------------------------------------------------------------------------------------
/// Mapping of our custom movement sub-modes
//---------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECustomMovementType : uint8
{
   /// Default custom movement type is none
   None     = 0   UMETA(DisplayName = "None"),

   /// Mantle / vault custom movement mode
   Mantle   = 1   UMETA(DisplayName = "Mantle"),

   /// Climb custom movement mode
   Scramble    = 2   UMETA(DisplayName = "Scramble"),

   /// Wall Climb custom movement mode
   WallClimb = 3   UMETA(DisplayName = "Wall Climb"),
};


//-----------------------------------------------------------------------------
/// Movement utilities
//-----------------------------------------------------------------------------

namespace OSE
{
   namespace MovementUtils
   {
      /// Helper function to get our custom movement type given the specified movement mode.
      /// This is used internally by the character movement component, but is helpful to
      /// expose when we want to query it without state.
      FORCEINLINE ECustomMovementType GetCustomMovementType(EMovementMode movementMode, uint8 customMode)
      {
         return (movementMode != EMovementMode::MOVE_Custom) ? ECustomMovementType::None : (ECustomMovementType)customMode;
      }
   }
}


//-----------------------------------------------------------------------------
/// A movement material context is essentially a wrapper around a
/// physical material and associated state copied from the movement component.
///
/// @NOTE! Only includes state used to meaningfully compare two
/// contexts for equality; avoid using data that changes frequently, such
/// as current location / rotation, etc!
//-----------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FOSEMovementMaterialContext
{
   GENERATED_BODY()

public:
   FOSEMovementMaterialContext()
      : IsMovingOnGround(false)
      , IsInAir(false)
      , IsScrambling(false)
      , IsClimbing(false)
      , IsMantling(false)
      , IsCrouching(false)
      , IsSprinting(false)
      , IsSliding(false)
   { }

   /// The star of the show!
   UPROPERTY(BlueprintReadOnly)
   TWeakObjectPtr<class UPhysicalMaterial> PhysicalMaterial;

   /// Mutually exclusive movement state
   UPROPERTY(BlueprintReadOnly) uint8 IsMovingOnGround : 1;  ///< UNavMovementComponent::IsMovingOnGround()
   UPROPERTY(BlueprintReadOnly) uint8 IsInAir : 1;           ///< UNavMovementComponent::IsFalling() || UNavMovementComponent::IsFlying()
   UPROPERTY(BlueprintReadOnly) uint8 IsScrambling : 1;        ///< UOSECharacterMovement::IsScrambling()
   UPROPERTY(BlueprintReadOnly) uint8 IsClimbing : 1;        ///< UOSECharacterMovement::IsWallClimbing()
   UPROPERTY(BlueprintReadOnly) uint8 IsMantling : 1;        ///< UOSECharacterMovement::IsMantling()


   /// State that might be set independent of movement state
   UPROPERTY(BlueprintReadOnly) uint8 IsCrouching : 1;       ///< UNavMovementComponent::IsCrouching()
   UPROPERTY(BlueprintReadOnly) uint8 IsSprinting : 1;       ///< UOSECharacterMovement::IsSprinting()
   UPROPERTY(BlueprintReadOnly) uint8 IsSliding : 1;         ///< UOSECharacterMovement::IsSliding()

   FORCEINLINE bool operator==(const FOSEMovementMaterialContext& other) const = default;
};
