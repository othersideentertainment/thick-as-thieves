// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// OSE
#include "Traversal/Jumping/OSEJumpSettings.h"
#include "OSEScrambleSettings.generated.h"


//---------------------------------------------------------------------------------------
/// Scrambling settings
//---------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FOSEScrambleSettings
{
   GENERATED_BODY()

public:

   /// Vertical velocity must be >= this value to start climbing
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal)
   float MinStartSpeed = -800;

   /// Vertical velocity must be >= this value to keep climbing
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal)
   float MinStopSpeed = 100;

   /// There must be at least this much head room to start climbing
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0))
   float MinCeilingClearance = 225;

   /// Constant vertical speed to use while climbing
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0))
   float ScrambleSpeed = 250;

   /// Percentage of climb speed to apply while climbing, regardless of input. The remainder of the
   /// climb speed (if any) will be applied based on input.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0, ClampMax = 1, UIMax = 1))
   float ScrambleSpeedBaseRatio = 0;

   /// Approximate height to reach when first starting climbing. This is applied as a boost to velocity
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0))
   float StartBoostHeight = 0;

   /// Duration in seconds for the climb sequence
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0))
   float Duration = 1.0f;

   /// How long we can go without confirming we're climbing. Once this expires, we stop climbing.
   /// This is needed since our impacts may not be reported frequently enough to rely on.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 0, UIMin = 0))
   float ExpirationDelay = 0.05f;

   /// Maximum angle in degrees for climbing surfaces facing the player
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = 1, UIMin = 1, ClampMax = 45, UIMax = 45))
   float MaxAngleFacing = 30.0f;

   /// Maximum camera (controller) pitch to set when climbing
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (ClampMin = -60, UIMin = -60, ClampMax = 60, UIMax = 60))
   float MaxCameraPitch = -30;

   /// Allows jumping while climbing when enabled
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal)
   bool JumpEnabled = false;

   /// The jump physics to use when jumping during a climb.
   /// Note; the values are in local space and applied while the character is still climbing.
   /// For example, the X component is positive into the wall, negative away from the wall.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Traversal, meta = (editcondition = "JumpEnabled"))
   FOSEJumpPhysics JumpPhysics;
};
