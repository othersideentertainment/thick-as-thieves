// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// OSE
#include "Traversal/Jumping/OSEJumpSettings.h"
#include "OSEWallClimbSettings.generated.h"


//---------------------------------------------------------------------------------------
/// Wall Climb settings
//---------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FOSEWallClimbSettings
{
   GENERATED_BODY()

public:

   /// Constant vertical speed to use while climbing
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0))
   float WallClimbSpeed = 250;

   /// Braking Acceleration applied while wall climbing
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0))
   float WallClimbBrakingAcceleration = 1500;

   /// Acceleration applied while wall climbing
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0))
   float WallClimbAcceleration = 1200;

   /// Degree angle maximum we consider for allowing movement while wall climbing between the wall normal and player look direction 
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0))
   float WallMovementAngleDegrees = 90.0f;

   /// How off center of the character is the wall allowed to be
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0))
   float DeflectionWallAngleDegrees = 25.0f;

   /// Minimum wall angle in degrees allowed for wall climbing
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0, ClampMax=360, UIMax=360))
   float MinClimbableWallAngle = 45.0f;

   /// Maximum wall angle in degrees allowed for wall climbing
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0, ClampMax = 360, UIMax = 360))
   float MaxClimbableWallAngle = 135.0f;

   /// Minimum angle between two walls allowed for traversal
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0, ClampMax = 360, UIMax = 360))
   float MinAngleCornerTraversal = 70.0f;

   /// Allow horizontal wall climb traversal around outside corners
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal)
   bool bAllowHorizontalCornerTraversal = false;

   /// Allow vertical wall climb traversal around outside corners
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal)
   bool bAllowVerticalCornerTraversal = false;

   /// Allows jumping while climbing when enabled
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal)
   bool JumpEnabled = false;

   // Magnitude that scales the acceleration of the Wall Climb Dash
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal)
   float WallClimbDashAcceleration = 1700;

   /// Duration in seconds wall climb dash lasts after it's initiated
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal)
   float WallClimbDashDuration = 0.5f;

   /// Player speed during wall climb dash
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal)
   float WallClimbDashSpeed = 500;

   /// Minimum sensitivity allowed when linearly slowing down look sensitivity as we get closer to looking towards wall climb movement target
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0.0, UIMin = 0.0, ClampMax = 1.0, UIMax = 1.0))
   float CameraLookTowardsMovementMinimumScalar = 0.4f;

   /// Maximum sensitivity allowed when linearly slowing down look sensitivity as we get closer to looking towards wall climb movement target
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0.5, UIMin = 0.5, ClampMax = 1.0, UIMax = 1.0))
   float CameraLookTowardsMovementMaximumScalar = 1.0f;

   /// Maximum radius of influence when slowing look sensitivity linearly towards our wall climb movement target (Degrees)
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0.0, UIMin = 0.0, ClampMax = 180.0, UIMax = 180.0))
   float CameraLookTowardsMovementRadiusDegrees = 40.0f;

   /// Minimum sensitivity allowed when linearly speeding up look sensitivity as we get closer to looking towards the wall
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0.5, UIMin = 0.5, ClampMax = 1.0, UIMax = 1.0))
   float CameraLookTowardsWallMinimumScalar = 1.0f;

   /// Maximum sensitivity allowed when linearly speeding up look sensitivity as we get closer to looking towards the wall
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 1.0, UIMin = 1.0, ClampMax = 2.0, UIMax = 2.0))
   float CameraLookTowardsWallMaximumScalar = 2.0f;

   /// Maximum radius of influence when speeding up look sensitivity linearly towards the wall (Degrees)
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0.0, UIMin = 0.0, ClampMax = 180.0, UIMax = 180.0))
   float CameraLookTowardsWallRadiusDegrees = 70.0f;

   /// Enables camera auto-turn. This points the camera towards the direction of movement during wall climb.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal)
   bool bEnableCameraAutoTurn = true;

   /// This dictates the speed of the lerp for wall climb camera auto-turn
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal)
   UCurveFloat* cameraAutoTurnSpeedCurve = nullptr;

   /// This dictates the minimum angle the camera auto-turn is allowed to activate (in degrees)
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal)
   float minimumCameraAutoTurnDegrees = 60.0f;

   /// Minimum amount of time in seconds where you're not allowed to reattach to a wall after manually canceling the climb
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal)
   float manuallyCanceledWallClimbCooldown = 1.0;

   /// Does attaching to a wall require an input direction towards that wall?
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal)
   bool bRequireInputToAttach = true;

   /// Is attaching to a wall allowed while walking?
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal)
   bool bNoWalkingWallAttachment = true;

   /// The jump physics to use when jumping during a climb.
   /// Note; the values are in local space and applied while the character is still climbing.
   /// For example, the X component is positive into the wall, negative away from the wall.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (editcondition = "JumpEnabled"))
   FOSEJumpPhysics JumpPhysics;
};
