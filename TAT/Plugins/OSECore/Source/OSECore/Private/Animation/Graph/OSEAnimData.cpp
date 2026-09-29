// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// ose
#include "Animation/Graph/OSEAnimData.h"
#include "Animation/Graph/OSEAnimActorInfo.h"
#include "Animation/Graph/OSEAnimDataLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAnimData)


//--------------------------------------------------------------------------------------------------
// Updates the angle and speed values (using the current values to derive speed)
//--------------------------------------------------------------------------------------------------

void FOSEAnimData_Rotation::Update(const FRotator& newRotation, float deltaT, float oneOverDeltaT, float interpSpeed)
{
   // Change in rotation
   const FRotator oldRotation = Angle;
   const FRotator deltaRotation = (newRotation - oldRotation).GetNormalized();

   // Store the new rotation value
   Angle = newRotation;

   // The (raw) rate of change in rotation is smoothed when stored
   const FRotator speedRotation = deltaRotation * oneOverDeltaT;
   Rate = FMath::RInterpTo(Rate, speedRotation, deltaT, interpSpeed);
}


//--------------------------------------------------------------------------------------------------
// Updates the vector values
//--------------------------------------------------------------------------------------------------

void FOSEAnimData_Vector::Update(const FVector& newVelocity)
{
   // Store velocity, direction, and magnitude (speed)
   Vector = newVelocity;
   Vector.ToDirectionAndLength(Direction, Length);

   // Update rotation
   Angle = Direction.ToOrientationRotator();
}


//--------------------------------------------------------------------------------------------------
// Updates the movement values
//--------------------------------------------------------------------------------------------------

void FOSEAnimData_Movement::Update(const FVector& worldVelocity, const FTransform& localToWorld)
{
   static const FVector kVectorMask2D = FVector(1.0f, 1.0f, 0.0f);
   
   Position.Update(localToWorld.GetLocation());

   // World space velocity
   Velocity.Update(worldVelocity);
   Velocity2D.Update(worldVelocity * kVectorMask2D);

   // Local space velocity
   const FVector localVelocity = localToWorld.InverseTransformVector(worldVelocity);
   LocalVelocity.Update(localVelocity);
   LocalVelocity2D.Update(localVelocity * kVectorMask2D);
}

void FOSEAnimData_Movement::Update(const FOSEAnimActorInfo& actorInfo)
{
   Update(actorInfo.ActorVelocity, actorInfo.ActorTransform);
}


//--------------------------------------------------------------------------------------------------
// Updates the input values
//--------------------------------------------------------------------------------------------------

void FOSEAnimData_Input::Update(const FVector& worldAcceleration, const FQuat& actorRotation, const FQuat& viewRotation)
{
   // World space acceleration
   Acceleration.Update(worldAcceleration);
   Acceleration2D.Update(worldAcceleration * FVector(1, 1, 0));

   // Local space acceleration
   const FVector localAcceleration = actorRotation.UnrotateVector(worldAcceleration);
   LocalAcceleration.Update(localAcceleration);
   LocalAcceleration2D.Update(localAcceleration * FVector(1, 1, 0));

   // View space acceleration
   const FVector viewAcceleration = viewRotation.UnrotateVector(worldAcceleration);
   ViewAcceleration.Update(viewAcceleration);
}

void FOSEAnimData_Input::Update(const FOSEAnimActorInfo& actorInfo)
{
   Update(actorInfo.Acceleration, actorInfo.ActorTransform.GetRotation(), actorInfo.ActorEyesRotation.Quaternion());
}


//--------------------------------------------------------------------------------------------------
// Updates environment data
//--------------------------------------------------------------------------------------------------

void FOSEAnimData_Environment::Update(const FFindFloorResult& floorResult, const FTransform& localToWorld)
{
   FRotator floorRotation;
   if (UOSEAnimDataFunctionLibrary::GetFloorRotation(floorRotation, floorResult, localToWorld))
   {
      // Only change the value if we were able to compute it (only if we were on a valid floor)
      FloorAngle = floorRotation;
   }
}

void FOSEAnimData_Environment::Update(const FOSEAnimActorInfo& actorInfo)
{
   Update(actorInfo.CurrentFloor, actorInfo.ActorTransform);

   FallingDistanceToFloor = actorInfo.CurrentFloorDistance;
   IsFallingInContactWithGround = actorInfo.MovementMode == EMovementMode::MOVE_Falling && actorInfo.TimeSinceLastImpact < 0.03;
   IsFacingWall = actorInfo.IsFacingWall;
   WallAngleLocal = actorInfo.ActorTransform.InverseTransformRotation(actorInfo.WallAngle.Quaternion()).Rotator();

}


//--------------------------------------------------------------------------------------------------
// Updates traversal data
//--------------------------------------------------------------------------------------------------

void FOSEAnimData_Traversal::Update(const FOSETraversalState& traversalState, const FVector& worldVelocity, const FVector& worldAcceleration, const FFindWallResult& wallResult)
{
   State = traversalState;
   IsMoving = worldVelocity.SizeSquared() > 0.0f;
   IsAccelerating = worldAcceleration.SizeSquared() > 0.0f;
   ClimbingPhysicalSurface = UOSEAnimDataFunctionLibrary::GetPhysicalSurfaceFromWall(wallResult);
}

void FOSEAnimData_Traversal::Update(const FOSEAnimActorInfo& actorInfo)
{
   Update(actorInfo.TraversalState, actorInfo.ActorVelocity, actorInfo.Acceleration, actorInfo.CurrentWall);
}


//--------------------------------------------------------------------------------------------------
// Updates turn-in-place data
//--------------------------------------------------------------------------------------------------

void FOSEAnimData_TurnInPlace::Update(const FOSETurnInPlaceState& turnInPlaceState)
{
   State = turnInPlaceState;
}

void FOSEAnimData_TurnInPlace::Update(const FOSEAnimActorInfo& actorInfo)
{
   Update(actorInfo.TurnInPlaceState);
}


//--------------------------------------------------------------------------------------------------
// Updates animation data
//--------------------------------------------------------------------------------------------------

void FOSEAnimData::Update(const FOSEAnimActorInfo& actorInfo)
{
   // Always update elapsed time for the current state, even if we eventually transition away
   TimeElapsed += actorInfo.DeltaT;

   // Get actor animation state
   State = UOSEAnimDataFunctionLibrary::GetAnimState(actorInfo.MovementMode, actorInfo.CustomMovementMode);

   // Get the relative aim rotation
   const FRotator actorRotation = actorInfo.ActorTransform.Rotator();
   const FRotator aimRotation = (actorInfo.ActorEyesRotation - actorRotation).GetNormalized();

   Rotation.Update(actorRotation, actorInfo.DeltaT, actorInfo.OneOverDeltaT, actorInfo.InterpSpeed);
   Aiming.Update(aimRotation, actorInfo.DeltaT, actorInfo.OneOverDeltaT, actorInfo.InterpSpeed);
   Movement.Update(actorInfo);
   Input.Update(actorInfo);
   Environment.Update(actorInfo);
   Traversal.Update(actorInfo);
   TurnInPlace.Update(actorInfo);
}

