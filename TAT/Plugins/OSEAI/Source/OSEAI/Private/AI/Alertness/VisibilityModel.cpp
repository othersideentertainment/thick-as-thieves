// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/Alertness/VisibilityModel.h"

// ue4
#include "Curves/CurveFloat.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VisibilityModel)

UCurveFloat* UVisibilityModel::GetScoreToTimeCurve(EAlertnessLevel alertnessLevel) const
{
   // This isn't my favorite way of doing this from a code standpoint (as
   // opposed to, say, just an array of curves). But it has the advantage of
   // keeping the individual curves surfaced and well-labelled in the editor
   // for designers to work with. (TJS)
   switch (alertnessLevel)
   {
   case EAlertnessLevel::Suspicious:
      return suspicionScoreCurve;
   case EAlertnessLevel::Alerted:
      return alertScoreCurve;
   default:
      return nullptr;
   }
}

float UVisibilityModel::WeightedFactors(float lighting, float motion, bool crouch, float distance, float periphery, FVisModelDebugLog& log) const
{
   float lightingFactor = FMath::Lerp(lightingFactorRange.lower, lightingFactorRange.upper, lighting);
   float motionFactor = FMath::Lerp(motionFactorRange.lower, motionFactorRange.upper, motion);
   float distanceFactor = FMath::Lerp(distanceFactorRange.lower, distanceFactorRange.upper, distance);
   float peripheryFactor = FMath::Lerp(peripheryFactorRange.lower, peripheryFactorRange.upper, periphery);
   float crouchFactor = crouch ? crouchFactorRange.upper : crouchFactorRange.lower;

#if WITH_GAMEPLAY_DEBUGGER
   log.DistanceFactor = distanceFactor;
   log.MotionFactor = motionFactor;
   log.PeripheryFactor = peripheryFactor;
   log.CrouchFactor = crouchFactor;
#endif

   return lightingFactor + motionFactor + distanceFactor + peripheryFactor + crouchFactor;
}

float UVisibilityModel::GetPeripheralVisionTransitionArc() const
{
   return peripheralVisionTransitionArc;
}

float UVisibilityModel::NormalizeSpeed(float cmPerSec)
{
   if (!speedToVisibilityCurve)
   {
      return 0.0f;
   }

   return FMath::Clamp(speedToVisibilityCurve->GetFloatValue(cmPerSec), 0.0f, 1.0f);
}

float UVisibilityModel::NormalizeDistance(float cmDistance)
{
   if (!distanceToVisibilityCurve)
   {
      return 0.0f;
   }

   return FMath::Clamp(distanceToVisibilityCurve->GetFloatValue(cmDistance), 0.0f, 1.0f);
}

float UVisibilityModel::ConvertScoreToRate(EAlertnessLevel alertnessLevel, float score)
{
   UCurveFloat* scoreCurve = GetScoreToTimeCurve(alertnessLevel);

   if (!scoreCurve)
   {
      return 0.0f;
   }

   float transitionTime = scoreCurve->GetFloatValue(score);
   if (transitionTime >= timeConsideredInfinite)
   {
      return 0.0f;
   }

   transitionTime = FMath::Max(transitionTime, 0.02f); // divide-by-zero protection

   return 1.0f / transitionTime;
}


