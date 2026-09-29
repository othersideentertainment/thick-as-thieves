// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Alertness/AlertnessEnums.h"

// ue4
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

// self
#include "VisibilityModel.generated.h"

class UCurveFloat;

USTRUCT()
struct FVisModelRange 
{
   GENERATED_USTRUCT_BODY()
public:
   UPROPERTY(EditAnywhere)
   float lower = 0.0f;

   UPROPERTY(EditAnywhere)
   float upper = 0.0f;
};

/// Contains all the inputs to a UVisibilityModel that are not relative to the
/// observer (so, for example, the viewed object's speed, but not distance)
USTRUCT(Blueprintable)
struct FVisModelAbsoluteInputs
{
   GENERATED_USTRUCT_BODY()
public:

   /// Speed I'm moving. AI should use apparentSpeed unless they have a reason not to.
   UPROPERTY(BlueprintReadWrite)
   float speed = 0.0f;

   /// TRUE if I'm crouched. AI should use isApparentlyCrouched unless they have a reason not to.
   UPROPERTY(BlueprintReadWrite)
   bool isCrouched = false;

   /// Speed I seem to be going for visibility purposes. May or may not be my true speed!
   UPROPERTY(BlueprintReadWrite)
   float apparentSpeed = 0.0f;

   /// If TRUE, then I'm as visible as if I were crouched (even if I'm not!)
   UPROPERTY(BlueprintReadWrite)
   bool isApparentlyCrouched = false;
};

/// Debug reporting information for how the visibility model came by its latest number.
USTRUCT(Blueprintable)
struct FVisModelDebugLog
{
   GENERATED_BODY()
public:

   /// How much of final score came from movement speed
   UPROPERTY(BlueprintReadWrite)
   float MotionFactor = 0.0f;

   /// How much of final score came from range
   UPROPERTY(BlueprintReadWrite)
   float DistanceFactor = 0.0f;

   /// How much of final score came from peripheral vision falloff
   UPROPERTY(BlueprintReadWrite)
   float PeripheryFactor = 0.0f;

   /// How much of final score came from posture
   UPROPERTY(BlueprintReadWrite)
   float CrouchFactor = 0.0f;

   /// Actual speed in cm/sec
   UPROPERTY(BlueprintReadWrite)
   float Speed = 0.0f;

   /// Actual distance in cm
   UPROPERTY(BlueprintReadWrite)
   float Distance = 0.0f;

   UPROPERTY(BlueprintReadWrite)
   float ViewAngle = 0.0f;

   /// World time these numbers were taken (not necessarily closely synched)
   UPROPERTY(BlueprintReadWrite)
   float Timestamp = -1.0f;
};

/// A UVisibilityModel is responsible for holding all of the parameters
/// and code describing how the factors that influence visibility
/// (e.g. light, motion, size, etc.) aggregate into a single metric
/// that AI can use to drive awareness.
UCLASS(Blueprintable)
class OSEAI_API UVisibilityModel : public UDataAsset
{
   GENERATED_BODY()
public:

   UPROPERTY(EditAnywhere)
   FVisModelRange lightingFactorRange;

   UPROPERTY(EditAnywhere)
   FVisModelRange motionFactorRange;

   UPROPERTY(EditAnywhere)
   FVisModelRange distanceFactorRange;

   UPROPERTY(EditAnywhere)
   FVisModelRange sizeFactorRange;

   UPROPERTY(EditAnywhere)
   FVisModelRange peripheryFactorRange;

   UPROPERTY(EditAnywhere)
   FVisModelRange crouchFactorRange;

   /// Degrees. Width of the arc where we smooth from direct to peripheral vision.
   UPROPERTY(EditAnywhere)
   float peripheralVisionTransitionArc;

   /// Vertical scale factor applied to whole vision cone. */
   UPROPERTY(EditAnywhere)
   float viewConeVerticalScale = 0.5f;

   /// Any transition time of at least this long will generate a _zero_ rate of change. */
   UPROPERTY(EditAnywhere)
   float timeConsideredInfinite;

   /// Mapping from illumination (in whatever units Unreal is using) to 0-1 visibility scoring factor. */
   UPROPERTY(EditAnywhere)
   UCurveFloat* lightToVisibilityCurve;

   /// Mapping from character speed (cm/sec) to 0-1 visibility scoring factor */
   UPROPERTY(EditAnywhere)
   UCurveFloat* speedToVisibilityCurve;

   /// Mapping from character distance (cm) to 0-1 visibility scoring factor */
   UPROPERTY(EditAnywhere)
   UCurveFloat* distanceToVisibilityCurve;

   /// Function taking in a weighted score, giving a time until the state transition from idle to suspicious */
   UPROPERTY(EditAnywhere)
   UCurveFloat* suspicionScoreCurve;

   /// Function taking in a weighted score, giving a time until the state transition from suspicious to alerted */
   UPROPERTY(EditAnywhere)
   UCurveFloat* alertScoreCurve;

   /// Angle (degrees) over which peripheral vision blends to direct vision in this VisibilityModel */
   UFUNCTION(BlueprintPure, Category = "AI | Vision")
   float GetPeripheralVisionTransitionArc() const;

   /// Convert real-world units (cm/sec) to the normalized 0-1 score to which this visibility model responds. */
   UFUNCTION(BlueprintPure, Category = "AI|Stealth")
   float NormalizeSpeed(float cmPerSec);

   /// Convert real-world units (cm) to the normalized 0-1 score to which this visibility model responds. */
   UFUNCTION(BlueprintPure, Category = "AI|Stealth")
   float NormalizeDistance(float cmDistance);

   /// Gives a weighted total visibility score according to the ranges and weights in the Visibilty Model.
   /// Fills in debug info in Log struct. 
   /// @param    Log    Debug info breaking down individual weights
   UFUNCTION(BlueprintPure, Category = "AI|Stealth")
   float WeightedFactors(float lighting, float motion, bool crouch, float distance, float periphery, FVisModelDebugLog& log) const;

   /// Translates exposure score into a rate of state transition.
   UFUNCTION(BlueprintPure, Category = "AI|Stealth")
   float ConvertScoreToRate(EAlertnessLevel alertnessLevel, float score);

private:

   /// Get float curve mapping weighted visibility score (unitless) to status-change time (sec)
   UCurveFloat* GetScoreToTimeCurve(EAlertnessLevel alertnessLevel) const;
};
