// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "OSEDistanceConstraint.generated.h"

enum class EOSEDistanceConstraintSurface : uint8
{
   Air,
   Ground
};

enum class EDistanceConstraintDelta : uint8
{
   None = 0,
   Partial = 1,
   Full = 2,
};

/// The distance constraint limits character movement
USTRUCT(BlueprintType)
struct OSECORE_API FOSEDistanceConstraint
{
   GENERATED_BODY()

   /// The position we constrain to.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (editcondition = "Enabled"))
   FVector Position = FVector::ZeroVector;

   /// The distance we constrain to.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (editcondition = "Enabled", ClampMin = "0", UIMin = "0"))
   float Distance = 0.0f;

   /// Additional slack we allow the distance to expand to
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (editcondition = "Enabled", HiddenByDefault, ClampMin = "0", UIMin = "0"))
   float SlackAmount = 0.0f;

   /// Amount of slack that has been used (just book-keeping)
   /// @TODO: have this be added to distance when considering the length?
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (editcondition = "Enabled", HiddenByDefault, ClampMin = "0", UIMin = "0"))
   float SlackUsed = 0.0f;

   /// The minimum length that the distance constraint is allowed to be shortened to when being controlled
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (editcondition = "Enabled", HiddenByDefault, ClampMin = "0", UIMin = "0"))
   float MinLength = 0.0f;

   /// The speed that we'll increase/decrease the distance based on the slack when controlling the distance
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (editcondition = "Enabled", HiddenByDefault, ClampMin = "0", UIMin = "0"))
   float ContractSpeed = 0.0f;

   /// The velocity that we'll increase the distance based on the slack when under tension in the air
   ///
   /// @TODO: Allow negative velocity to retract the distance?
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (editcondition = "Enabled", HiddenByDefault, ClampMin = "0", UIMin = "0"))
   float SlackVelocityAir = 0.0f;

   /// The velocity that we'll increase the distance based on the slack when under tension on the ground
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (editcondition = "Enabled", HiddenByDefault, ClampMin = "0", UIMin = "0"))
   float SlackVelocityGround = 0.0f;

   /// The braking deceleration to use when affected by the distance constraint
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (editcondition = "Enabled", HiddenByDefault, ClampMin = "0", UIMin = "0"))
   float BrakingDeceleration = 0.0f;

   /// This value is stored in the struct intentionally. If the struct
   /// is replicated AND this value is used to disallow move combining
   /// in character movement, then it is likely the struct was
   /// correctly replicated to the server (and/or set locally on the
   /// autonomous proxy) and our predicted, actual, and saved moves
   /// are more likely to remain in sync.
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   bool Enabled = false;

   /// Utility function to increase the distance based on slack. Returns the amount of
   /// additional slack added to the distance. Will be zero if no slack is available.
   float IncreaseSlack(float slackVelocity, float deltaSeconds, float desiredExtra)
   {
      if (SlackAmount <= KINDA_SMALL_NUMBER)
         return 0.0f;

      // Actual delta
      const float deltaAmount = FMath::Min(slackVelocity * deltaSeconds, FMath::Min(SlackAmount, desiredExtra));

      // Distribute the slack
      Distance += deltaAmount;
      SlackUsed += deltaAmount;
      SlackAmount -= deltaAmount;
      return deltaAmount;
   }

   float ClampSlack(float newDistance) const
   {
      return FMath::Clamp(newDistance, FMath::Min(Distance, MinLength), Distance + SlackAmount);
   }

   float ForceSlack(float newDistance)
   {
      newDistance = ClampSlack(newDistance);
      const float distanceDelta = newDistance - Distance;
      SlackUsed += distanceDelta;
      SlackAmount -= distanceDelta;
      Distance = newDistance;
      return distanceDelta;
   }

   bool CanContract() const { return ContractSpeed > 0; }
   bool CanExtend() const
   {
      return SlackVelocityAir > 0 || SlackVelocityGround > 0;
   }

   EDistanceConstraintDelta GetDeltaTo(const FOSEDistanceConstraint& other) const;

   void Apply(EDistanceConstraintDelta type, const FOSEDistanceConstraint& other);

   void SerializeDelta(EDistanceConstraintDelta type, FArchive& ar);
   void SerializeFull(FArchive& ar);
   void SerializePartial(FArchive& ar);

   float SlackVelocityForSurface(EOSEDistanceConstraintSurface surface) const
   {
      switch (surface)
      {
      case EOSEDistanceConstraintSurface::Air:
         return SlackVelocityAir;
      case EOSEDistanceConstraintSurface::Ground:
         return SlackVelocityGround;
      default:
         check(false);
         return 0;
      }
   }
};
