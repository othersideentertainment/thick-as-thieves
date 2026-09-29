// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE4
#include "CoreMinimal.h"

// OSE
#include "Animation/Graph/OSEAnimTypes.h"
#include "Character/OSECharacterMovement.h"
#include "Character/OSECharacterRotation.h"
#include "Traversal/TraversalInterface.h"
#include "OSEAnimData.generated.h"

struct FOSEAnimActorInfo;


//--------------------------------------------------------------------------------------------------
/// Common class to support rotation (FRotator) data, conversion, and speed
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FOSEAnimData_Rotation
{
   GENERATED_BODY()

public:

   /// The actual angle (in degrees) from which all members are derived
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FRotator Angle = FRotator::ZeroRotator;

   /// The angular velocity, which is the change in angle over time (degrees per second)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FRotator Rate = FRotator::ZeroRotator;

public:

   /// Implicit conversion operator; returns the angle value
   FORCEINLINE operator FRotator() const { return Angle; }

   /// Updates the angle and speed values (using the current values to derive speed)
   void Update(const FRotator& newRotation, float deltaT, float oneOverDeltaT, float interpSpeed);
};


//--------------------------------------------------------------------------------------------------
/// Common class for vector animation data. This can be used to provide useful data to animations
/// while easily supporting different coordinate spaces and 2D representations.
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FOSEAnimData_Vector
{
   GENERATED_BODY()

public:

   /// The actual velocity vector from which all other members are derived
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FVector Vector = FVector::ZeroVector;

   /// Velocity direction (normalized velocity vector)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FVector Direction = FVector::ZeroVector;

   /// Velocity direction as a rotation (yaw and pitch angle in degrees; roll is always zero)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FRotator Angle = FRotator::ZeroRotator;

   /// Total speed (velocity magnitude)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   float Length = 0.0f;

public:

   /// Implicit conversion operator; returns the velocity value
   FORCEINLINE operator FVector() const { return Vector; }

   /// Updates the velocity values
   void Update(const FVector& newVector);
};


//--------------------------------------------------------------------------------------------------
/// Abstract base class for all animation data structs
//--------------------------------------------------------------------------------------------------

USTRUCT()
struct OSECORE_API FOSEBaseAnimData
{
   GENERATED_BODY()

public:

   FOSEBaseAnimData() { }
   virtual ~FOSEBaseAnimData() { }

   virtual void Update(const FOSEAnimActorInfo& actorInfo) { unimplemented(); }
};


//--------------------------------------------------------------------------------------------------
/// Movement animation data. Used to provide velocity data to animations in different coordinate
/// spaces and 2D representations.
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FOSEAnimData_Movement : public FOSEBaseAnimData
{
   GENERATED_BODY()

public:

   /// Positon (3D; world space)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSEAnimData_Vector Position;

   /// Velocity (3D; world space)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSEAnimData_Vector Velocity;

   /// Velocity (2D; world space)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSEAnimData_Vector Velocity2D;

   /// Velocity (3D; local space)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSEAnimData_Vector LocalVelocity;

   /// Velocity (2D; local space)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSEAnimData_Vector LocalVelocity2D;

   /// Displacement since last frame (3D; world space)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSEAnimData_Vector Displacement;

   /// Displacement since last frame (2D; world space)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSEAnimData_Vector Displacement2D;

   /// 2d velocity based on displacement  (2D; world space)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSEAnimData_Vector DisplacementVelocity2D;

public:

   /// Updates the movement values
   void Update(const FVector& worldVelocity, const FTransform& localToWorld);
   virtual void Update(const FOSEAnimActorInfo& actorInfo) override;
};


//--------------------------------------------------------------------------------------------------
/// Input (acceleration) animation data. Used to provide acceleration data to animations in
/// different coordinate spaces and 2D representations.
/// NOTE: When input is not replicated (simulated proxies), it is usually the same as the velocity.
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FOSEAnimData_Input : public FOSEBaseAnimData
{
   GENERATED_BODY()

public:

   /// Input acceleration (3D; world space)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSEAnimData_Vector Acceleration;

   /// Input acceleration (2D; world space)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSEAnimData_Vector Acceleration2D;

   /// Input acceleration (3D; local space)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSEAnimData_Vector LocalAcceleration;

   /// Input acceleration (2D; local space)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSEAnimData_Vector LocalAcceleration2D;

   /// Input acceleration (3D; view/eye space)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSEAnimData_Vector ViewAcceleration;

public:

   /// Updates the input values
   void Update(const FVector& worldAcceleration, const FQuat& actorRotation, const FQuat& viewRotation);
   virtual void Update(const FOSEAnimActorInfo& actorInfo) override;
};


//--------------------------------------------------------------------------------------------------
/// Environment animation data
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FOSEAnimData_Environment : public FOSEBaseAnimData
{
   GENERATED_BODY()

public:

   /// Floor angle (only valid when on a walkable floor)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FRotator FloorAngle = FRotator::ZeroRotator;

   /// Distance to floor (only valid when falling)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   float FallingDistanceToFloor = 0;

   /// While in the Falling move mode, if the character is in contact with the ground and slipping
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   bool IsFallingInContactWithGround = false;

   /// While in the Wall Climb move mode, if the character is facing the wall
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   bool IsFacingWall = true;

   /// Wall angle (only valid when wall climbing)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FRotator WallAngleLocal = FRotator::ZeroRotator;

public:

   /// Updates environment data
   void Update(const struct FFindFloorResult& floorResult, const FTransform& localToWorld);
   virtual void Update(const FOSEAnimActorInfo& actorInfo) override;
};


//--------------------------------------------------------------------------------------------------
/// Traversal animation data
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FOSEAnimData_Traversal : public FOSEBaseAnimData
{
   GENERATED_BODY()

public:

   FOSEAnimData_Traversal()
      : FOSEBaseAnimData()
      , IsMoving(false)
      , IsAccelerating(false)
   { }

   /// Combined traversal state; queried from the traversal interface
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSETraversalState State;

   /// True when moving (velocity length > 0)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   uint8 IsMoving : 1;

   /// True when acceleration isn't 0. On simulated proxies (other peoples' characters) this will be the same as IsMoving, since acceleration isn't replicated but is the direction of the velocity.
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   uint8 IsAccelerating : 1;

   /// The current Physical Surface we are on when climbing.  Set to EPhysicalSurface::SurfaceType_Default when not climbing or when there is no Phys Material in CurrentWall.
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TEnumAsByte<EPhysicalSurface> ClimbingPhysicalSurface = SurfaceType_Default;

public:

   /// Updates traversal data
   void Update(const FOSETraversalState& traversalState, const FVector& worldVelocity, const FVector& worldAcceleration, const FFindWallResult& wallResult);
   virtual void Update(const FOSEAnimActorInfo& actorInfo) override;
};


//--------------------------------------------------------------------------------------------------
/// Turn-in-place animation data
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FOSEAnimData_TurnInPlace : public FOSEBaseAnimData
{
   GENERATED_BODY()

public:

   FOSEAnimData_TurnInPlace()
      : FOSEBaseAnimData()
   { }

   /// The raw turn-in-place state queried from the movement component
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSETurnInPlaceState State;

public:

   /// Updates turn-in-place data
   void Update(const FOSETurnInPlaceState& turnInPlaceState);
   virtual void Update(const FOSEAnimActorInfo& actorInfo) override;
};

//--------------------------------------------------------------------------------------------------
/// Animation-specific representation of character state data. Suitable for use in animation graphs.
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FOSEAnimData : public FOSEBaseAnimData
{
   GENERATED_BODY()

public:

   /// The state associated with this data
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   EOSEAnimState State = EOSEAnimState::Unknown;

   /// The total time spent in this state
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   float TimeElapsed = 0.0f;

   /// Actor orientation data
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSEAnimData_Rotation Rotation;

   /// Controller orientation data
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSEAnimData_Rotation Aiming;

   /// Movement data
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSEAnimData_Movement Movement;

   /// Input / acceleration data
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSEAnimData_Input Input;

   /// Environment data
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSEAnimData_Environment Environment;

   /// Traversal data
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSEAnimData_Traversal Traversal;

   /// Turn-in-place data
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FOSEAnimData_TurnInPlace TurnInPlace;

public:

   virtual void Update(const FOSEAnimActorInfo& actorInfo) override;
};
