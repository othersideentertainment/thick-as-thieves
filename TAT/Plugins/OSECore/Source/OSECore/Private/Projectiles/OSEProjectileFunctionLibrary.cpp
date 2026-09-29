// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Projectiles/OSEProjectileFunctionLibrary.h"

// ose
#include "Projectiles/OSEProjectileFunctionLibraryTypes.h"

// ue5
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "DrawDebugHelpers.h"
#include "WorldCollision.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEProjectileFunctionLibrary)

// NOTE: adapted from UGameplayStatics::PredictProjectilePath, but with support for collison profiles
bool UOSEProjectileFunctionLibrary::PredictProjectilePath(const UObject* worldContextObject, const FOSEPredictProjectilePathParams& predictParams, FPredictProjectilePathResult& predictResult)
{
   predictResult.Reset();
   bool blockingHit = false;

   UWorld const* const world = GEngine->GetWorldFromContextObject(worldContextObject, EGetWorldErrorMode::LogAndReturnNull);
   if (world && predictParams.SimFrequency > KINDA_SMALL_NUMBER)
   {
      const float substepDeltaTime = 1.f / predictParams.SimFrequency;
      const float gravityZ = FMath::IsNearlyEqual(predictParams.OverrideGravityZ, 0.0f) ? world->GetGravityZ() : predictParams.OverrideGravityZ;
      const float projectileRadius = predictParams.ProjectileRadius;

      FCollisionQueryParams queryParams(SCENE_QUERY_STAT(OSEPredictProjectilePath), predictParams.bTraceComplex);

      ECollisionChannel traceChannel = ECC_WorldStatic;
      FCollisionResponseParams responseParams = FCollisionResponseParams::DefaultResponseParam;
      const bool bTracePath = predictParams.bTraceWithCollision && (predictParams.bTraceWithProfile);
      if (bTracePath)
      {
         queryParams.AddIgnoredActors(predictParams.ActorsToIgnore);
         if (predictParams.bTraceWithProfile)
         {
            UCollisionProfile::GetChannelAndResponseParams(predictParams.TraceProfile, traceChannel, responseParams);
         }
      }

      FVector currentVel = predictParams.LaunchVelocity;
      FVector traceStart = predictParams.StartLocation;
      FVector traceEnd = traceStart;
      float currentTime = 0.f;
      predictResult.PathData.Reserve(FMath::Min(128, FMath::CeilToInt(predictParams.MaxSimTime * predictParams.SimFrequency)));
      predictResult.AddPoint(traceStart, currentVel, currentTime);

      FHitResult traceHit(NoInit);
      traceHit.Time = 1.f;

      const float maxSimTime = predictParams.MaxSimTime;
      while (currentTime < maxSimTime)
      {
         // Limit step to not go further than total time.
         const float previousTime = currentTime;
         const float actualStepDeltaTime = FMath::Min(maxSimTime - currentTime, substepDeltaTime);
         currentTime += actualStepDeltaTime;

         // Integrate (Velocity Verlet method)
         traceStart = traceEnd;
         const FVector oldVelocity = currentVel;
         currentVel = oldVelocity + FVector(0.f, 0.f, gravityZ * actualStepDeltaTime);
         traceEnd = traceStart + (oldVelocity + currentVel) * (0.5f * actualStepDeltaTime);
         predictResult.LastTraceDestination.Set(traceEnd, currentVel, currentTime);

         if (bTracePath)
         {
            bool hadHit = world->SweepSingleByChannel(traceHit, traceStart, traceEnd, FQuat::Identity, traceChannel, FCollisionShape::MakeSphere(projectileRadius), queryParams, responseParams);

            // See if there were any hits.
            if (hadHit)
            {
               // Hit! We are done. Choose trace with earliest hit time.
               predictResult.HitResult = traceHit;
               const float hitTimeDelta = actualStepDeltaTime * traceHit.Time;
               const float totalTimeAtHit = previousTime + hitTimeDelta;
               const FVector velocityAtHit = oldVelocity + FVector(0.f, 0.f, gravityZ * hitTimeDelta);
               predictResult.AddPoint(predictResult.HitResult.Location, velocityAtHit, totalTimeAtHit);
               blockingHit = true;
               break;
            }
         }

         predictResult.AddPoint(traceEnd, currentVel, currentTime);
      }

      // Draw debug path
#if ENABLE_DRAW_DEBUG
      if (predictParams.DrawDebugType != EDrawDebugTrace::None)
      {
         const bool bPersistent = predictParams.DrawDebugType == EDrawDebugTrace::Persistent;
         const float LifeTime = (predictParams.DrawDebugType == EDrawDebugTrace::ForDuration) ? predictParams.DrawDebugTime : 0.f;
         const float DrawRadius = (projectileRadius > 0.f) ? projectileRadius : 5.f;

         // draw the path
         for (const FPredictProjectilePathPointData& pathPt : predictResult.PathData)
         {
            ::DrawDebugSphere(world, pathPt.Location, DrawRadius, 12, FColor::Green, bPersistent, LifeTime);
         }
         // draw the impact point
         if (blockingHit)
         {
            ::DrawDebugSphere(world, predictResult.HitResult.Location, DrawRadius + 1.0f, 12, FColor::Red, bPersistent, LifeTime);
         }
      }
#endif //ENABLE_DRAW_DEBUG
   }

   return blockingHit;
}

// BP wrapper to general-purpose function.
bool UOSEProjectileFunctionLibrary::Blueprint_PredictProjectilePath_ByTraceProfile(
   const UObject* worldContextObject,
   FHitResult& outHit,
   TArray<FVector>& outPathPositions,
   FVector& outLastTraceDestination,
   FVector startPos,
   FVector launchVelocity,
   bool bTracePath,
   float projectileRadius,
   FName traceProfile,
   bool traceComplex,
   const TArray<AActor*>& actorsToIgnore,
   EDrawDebugTrace::Type drawDebugType,
   float drawDebugTime,
   float simFrequency,
   float maxSimTime,
   float overrideGravityZ)
{
   FOSEPredictProjectilePathParams params(projectileRadius, startPos, launchVelocity, maxSimTime);
   params.bTraceWithCollision = bTracePath;
   params.bTraceComplex = traceComplex;
   params.ActorsToIgnore = actorsToIgnore;
   params.DrawDebugType = drawDebugType;
   params.DrawDebugTime = drawDebugTime;
   params.SimFrequency = simFrequency;
   params.OverrideGravityZ = overrideGravityZ;
   params.bTraceWithProfile = true;
   params.TraceProfile = traceProfile;

   // Do the trace
   FPredictProjectilePathResult predictResult;
   const bool bHit = PredictProjectilePath(worldContextObject, params, predictResult);

   // Fill in results.
   outHit = predictResult.HitResult;
   outLastTraceDestination = predictResult.LastTraceDestination.Location;
   outPathPositions.Empty(predictResult.PathData.Num());
   for (const FPredictProjectilePathPointData& pathPoint : predictResult.PathData)
   {
      outPathPositions.Add(pathPoint.Location);
   }
   return bHit;
}

bool UOSEProjectileFunctionLibrary::PlotInterceptCourse(FVector sourcePoint, float interceptorSpeed, FVector targetPosition, FVector targetVelocity, FVector& course)
{
   QUICK_SCOPE_CYCLE_COUNTER(UOSEProjectileFunctionLibrary_PlotInterceptCourse);

   float interceptTime = PlotInterceptTime(sourcePoint, interceptorSpeed, targetPosition, targetVelocity, FVector::ZeroVector);

   bool canIntercept = (!FMath::IsNaN(interceptTime) && interceptTime >= 0.0f);
   if (!canIntercept)
   {
      // No intercept course possible. Instead, use an approximate course that at least 
      // looks like you're trying, based on travel time to target's _current_ position.
      interceptTime = targetPosition.Size() / interceptorSpeed;
   }

   course = targetPosition + interceptTime * targetVelocity;
   course.Normalize();

   return canIntercept;
}

float UOSEProjectileFunctionLibrary::PlotInterceptTime(FVector sourcePoint, float interceptorSpeed, FVector targetPosition, FVector targetVelocity, FVector targetAcceleration)
{
   QUICK_SCOPE_CYCLE_COUNTER(UOSEProjectileFunctionLibrary_PlotInterceptTime);

   float interceptTime = 0.0f;

   // Transform into a coordinate space with SourcePoint at the origin.
   targetPosition = targetPosition - sourcePoint;

   /* To understand this, it helps to imagine solving the easier problem where
      the interceptor is a shockwave moving out in all directions at a known
      speed, where we're just trying to figure out the intercept _time_. Given
      that, we can figure out the target's position at that time, and infer the
      direction that a non-omnidirectional interceptor would have to aim.

      Assume zero acceleration. The moving target's position is quadratic in time. 
      So, the squared length of its distance from the origin is a quadratic equation, 
      as is the difference between it and the squared distance of our hypothetical shockwave.

      We can solve that quadratic equation in the usual way.

      The case with nonzero acceleration is the same, except that the target's position
      becomes quadratic with time, so the squared distance that gives us the equation
      we need to solve becomes a quartic. But there's a general solution for those, too.
   */
   FPolynomialSolutions possibleInterceptTimes;
   float coef2 = FVector::DotProduct(targetVelocity, targetVelocity) - interceptorSpeed * interceptorSpeed;
   float coef1 = 2.0f * FVector::DotProduct(targetPosition, targetVelocity);
   float coef0 = FVector::DotProduct(targetPosition, targetPosition);

   // Notice that higher-order terms are zero if there's no acceleration,
   // and there's a lot less potential for numerical errors if we cut straight to 
   // the quadratic formula.
   if (FMath::IsNearlyZero(targetAcceleration.SizeSquared()))
   {
      possibleInterceptTimes = _SolveQuadratic(coef2, coef1, coef0);
   }
   else
   {
      // @NOTE: There's a lot of calculation in finding the roots of quartics analytically,
      // which might be too heavy for something AI are doing all the time. If so, it might
      // be just as good to estimate using the quadratic solution (above), and then run it
      // through a few rounds of Newton's method to get a good approximation.
      coef2 += FVector::DotProduct(targetAcceleration, targetPosition);
      float coef4 = 0.25f * FVector::DotProduct(targetAcceleration, targetAcceleration);
      float coef3 = FVector::DotProduct(targetVelocity, targetAcceleration);
      possibleInterceptTimes = _SolveQuartic(coef4, coef3, coef2, coef1, coef0);
   }
   // If no real solutions found, we're done.
   if (possibleInterceptTimes.Num() < 1)
   {
      return NAN;
   }

   // we want the smallest _nonnegative_ intercept time. In fact, we'll typically
   // get one solution in the past and one in the future.
   possibleInterceptTimes.Sort();
   for (int i = 0; i < possibleInterceptTimes.Num(); i++)
   {
      if (possibleInterceptTimes[i] >= 0.0f)
      {
         return possibleInterceptTimes[i];
      }
   }

   // Only negative solutions for some reason.
   return NAN; 
}

float UOSEProjectileFunctionLibrary::GetProjectileSpeedByClass(UClass* projectileClass)
{
   UProjectileMovementComponent* moveComponent = GetProjectileMovementByClass(projectileClass);
   if (IsValid(moveComponent))
   {
      return moveComponent->InitialSpeed;
   }
   return -1.0f;
}

UProjectileMovementComponent* UOSEProjectileFunctionLibrary::GetProjectileMovementByClass(UClass* projectileClass)
{
   if (IsValid(projectileClass))
   {
      UObject* defaultObject = projectileClass->GetDefaultObject();
      if (IsValid(defaultObject))
      {
         AActor* defaultObjectAsActor = Cast<AActor>(defaultObject);
         if (IsValid(defaultObjectAsActor))
         {
            UProjectileMovementComponent* moveComponent = defaultObjectAsActor->FindComponentByClass<UProjectileMovementComponent>();
            if (IsValid(moveComponent))
            {
               return moveComponent;
            }
         }
      }
   }
   return nullptr;
}

// Note that the a, b, c here are standard conventions for the coefficients of a quadratic
// equation, not just lazy variable names. They're _those_ a, b, and c.
UOSEProjectileFunctionLibrary::FPolynomialSolutions UOSEProjectileFunctionLibrary::_SolveQuadratic(float a, float b, float c)
{
   FPolynomialSolutions realSolutions;

   // Check if we reduce to a linear equation (or even a constant)
   if (a == 0.0f)
   {
      if (b == 0.0f)
      {
         if (c == 0.0f)
         {
            // *All* real numbers are solutions to 0x^2 + 0x + 0 = 0. This return value is arbitrary.
            realSolutions.Add(0.0f);
            return realSolutions;
         }
         // no solution possible
         return realSolutions;
      }

      realSolutions.Add(-c / b);
      return realSolutions;
   }

   b /= a;
   c /= a;

   if (!FMath::IsFinite(b) || !FMath::IsFinite(c))
   {
      // This happens if b and/or c was tremendous compared to a. So,
      // treat this as a linear equation (a=0)
      float solution = -c / b;
      if (!FMath::IsFinite(solution) || b == 0.0f)
      {
         // Huh. c was also tremendous compared to b.
         if (FMath::IsNearlyZero(c))
         {
            // All real numbers are solutions to 0x^2+0x+0=0. This return value is arbitrary.
            realSolutions.Add(0.0f);
            return realSolutions;
         }
         else
         {
            return realSolutions; // No solutions exist
         }
      }
      realSolutions.Add(-c / b);
   }

   float discriminant = b * b - 4.0 * c;
   if (discriminant < 0.0f)
   {
      // no non-imaginary solutions.
      return realSolutions;
   }

   float vertex = -b / 2.0f;
   float reducedDiscriminant = FMath::Sqrt(discriminant) / 2.0f;

   if (FMath::IsNearlyZero(reducedDiscriminant))
   {
      realSolutions.Add(vertex);
      return realSolutions;
   }

   // This solution is numerically preferable in the face of the b^2 >> 4ac case.
   float stableSign = (b >= 0.0f) ? 1.0f : -1.0f; // Using Sign(b) would leave the possibility of 0, which is bad.
   float stableSolution = vertex - stableSign * reducedDiscriminant;
   float otherSolution = vertex + vertex - stableSolution;

   realSolutions.Add(stableSolution);
   realSolutions.Add(otherSolution);

   return realSolutions;
}

/* Find real solutions to the cubic equation with coefficients Coef3 (cubic term), etc.
*/
UOSEProjectileFunctionLibrary::FPolynomialSolutions UOSEProjectileFunctionLibrary::_SolveCubic(float coef3, float coef2, float coef1, float coef0)
{
   // A change of variable can eliminate the quadratic term, so we can then
   // solve this as a "depressed" cubic. But that relies on Coef3 != 0, so check that first.
   if (coef3 == 0.0f)
   {
      return _SolveQuadratic(coef2, coef1, coef0);
   }

   float depressedCoef1 = (3.0f * coef3 * coef1 - coef2 * coef2) / (3.0f * coef3 * coef3);
   float depressedCoef0 = ((2.0f/27.0f) * coef2 * coef2 * coef2 - (1.0f/3.0f) * coef3 * coef2 * coef1 + coef3 * coef3 * coef0) / (coef3 * coef3 * coef3);
   float changeOfVariableDifference = (coef2 / 3.0f) / coef3;
   // If leading coefficient is very _small_ compared to others, we may have overflowed
   // here, but that, again, means we basically have a quadratic equation.
   if (!(FMath::IsFinite(depressedCoef0) && FMath::IsFinite(depressedCoef1) && FMath::IsFinite(changeOfVariableDifference)))
   {
      float scale = FMath::Max3(FMath::Abs(coef2), FMath::Abs(coef1), FMath::Abs(coef0));
      coef2 /= scale;
      coef1 /= scale;
      coef0 /= scale;
      return _SolveQuadratic(coef2, coef1, coef0);
   }

   FPolynomialSolutions depressedCubicSolutions = _SolveDepressedCubic(depressedCoef1, depressedCoef0);

   // Reverse change of variable to get roots of the original equation.
   for (int i = 0; i < depressedCubicSolutions.Num(); i++)
   {
      depressedCubicSolutions[i] -= changeOfVariableDifference;
   }
   return depressedCubicSolutions;
}

/* Find real solutions to a so-called "depressed" cubic equation of the standard form
   x^3+px+q = 0. Using the p,q naming convention here because that's what you'll find
   in almost all references about the Cubic Formula.
*/
UOSEProjectileFunctionLibrary::FPolynomialSolutions UOSEProjectileFunctionLibrary::_SolveDepressedCubic(float p, float q)
{
   // Assume no solutions until proven otherwise (though there will always be at least 1 for a cubic)
   FPolynomialSolutions realSolutions;

   if (FMath::IsNearlyZero(q))
   {
      // To a good approximation, this is secretly the quadratic x^2+p = 0, or x=0
      realSolutions = _SolveQuadratic(1.0f, 0.0f, p);
      realSolutions.Add(0.0f);
      return realSolutions;
   }
   if (FMath::IsNearlyZero(p))
   {
      // To a good approximation, this is just x^3+q = 0
      realSolutions.Add(_CubeRoot(-q));
      return realSolutions;
   }

   float halfQ = 0.5f * q;
   float thirdP = p / 3.0f;
   float discriminant = halfQ * halfQ + thirdP * thirdP * thirdP;

   // Overflow indicates that P and/or Q is tremendous,
   // so the leading cubic term is negligible.
   // Call it px+q = 0, then.
   if (!FMath::IsFinite(discriminant))
   {
      // There are some special cases here with degenerate values of p and q, but
      // the routine for solving quadratics already handles those cases, so we're
      // using that even though we already know the x^2 coefficient is 0.
      return _SolveQuadratic(0.0f, p, q);
   }

   if (FMath::IsNearlyZero(discriminant))
   {
      // We have three real roots, but only two of them distinct
      realSolutions.Add(3.0f * q / p);
      realSolutions.Add(-0.5f * realSolutions[0]);
      return realSolutions;
   }
   if (discriminant > 0.0f)
   {
      // We have a single real root, available via Cardano's Formula
      float discriminantRoot = FMath::Sqrt(discriminant);
      realSolutions.Add(_CubeRoot(-halfQ + discriminantRoot) + _CubeRoot(-halfQ - discriminantRoot));
      return realSolutions;
   }

   // In this case, the terms of Cardano's formula give us a pair
   // of complex conjugates that cancel to a real number, but it'll take
   // some extra work deriving the complex cube root to get there.
   float intermediatePolarAngle = FMath::Acos((halfQ / thirdP) * FMath::Sqrt(-1.0 / thirdP)); // note thirdP is guaranteed negative at this point
   float rootTermMagnitude = FMath::Sqrt(-thirdP);
   float rootTermAngle = intermediatePolarAngle / 3.0f;

   // Cube root of this intermediate value is just the same magnitude but 1/3 angle.
   // Note that the imaginary parts will cancel out (complex conjugates, yo), so
   // we just care about the cosine of the fractional angle (twice, once for each conjugate).
   realSolutions.Add(2.0f * rootTermMagnitude * FMath::Cos(rootTermAngle));

   // Additional roots come from other conjugate pairs separated by 1/3 circle (cube roots of unity)
   // Another option, given one solution, would be to reduce the equation to a quadratic and go from there.
   realSolutions.Add(2.0f * rootTermMagnitude * FMath::Cos(rootTermAngle + PI * (2.0f/3.0f)));
   realSolutions.Add(2.0f * rootTermMagnitude * FMath::Cos(rootTermAngle + PI * (4.0f/3.0f)));

   return realSolutions;
}

UOSEProjectileFunctionLibrary::FPolynomialSolutions UOSEProjectileFunctionLibrary::_SolveQuartic(float coef4, float coef3, float coef2, float coef1, float coef0)
{
   // Eliminate the cubic term with a change of variable, and solve the resulting depressed quartic

   // normalize everything to a leading coefficent of 1.
   // This could all go horribly awry if coef4 is zero (or nearly zero, with other coefficients of large
   // magnitude), but it usually won't, and we'll check for that in a bit.
   // Using x^4+bx^3+cx^2+dx+e naming convention for coefficients here because that's what you're
   // likely to find in the literature, which makes checking the formulas below easier.
   float b = coef3 / coef4;
   float c = coef2 / coef4;
   float d = coef1 / coef4;
   float e = coef0 / coef4;

   // Find the coefficients of the quartic equation arrived at via change of variable to
   // eliminate the x^3 term (the "depressed quartic").
   float depressedCoef2 = (b * b) * (-3.0f / 8.0f) + c;
   float depressedCoef1 = (b * b * b) * 0.125f - 0.5f * (b * c) + d;
   float depressedCoef0 = (b * b) * 0.0625f;
   depressedCoef0 = (-3.0f * depressedCoef0 * depressedCoef0) + (depressedCoef0 * c) - (0.25f * b * d) + e;

   // Okay, did everything go horribly awry? if so, that means the quartic term was so small compared to
   // the others (possibly zero) that we can just treat this thing as a cubic.
   if (coef4 == 0.0f ||
      !FMath::IsFinite(depressedCoef2) ||
      !FMath::IsFinite(depressedCoef1) ||
      !FMath::IsFinite(depressedCoef0))
   {
      return _SolveCubic(coef3, coef2, coef1, coef0);
   }

   // Biquadratic equations (i.e., seeming cubics that are actually quadratic in x^2)
   // come up a lot in practice (e.g. ballistic equations for non-moving targets).
   // So, check for that case first.
   if (FMath::IsNearlyZero(coef3) && FMath::IsNearlyZero(coef1))
   {
      return _SolveBiquadratic(1.0f, c, e);
   }

   // With these coefficients, we have a depressed quartic
   FPolynomialSolutions realSolutions = _SolveDepressedQuartic(depressedCoef2, depressedCoef1, depressedCoef0);

   float changeOfVariableOffset = -0.25f * b;
   for (int i = 0; i < realSolutions.Num(); i++)
   {
      realSolutions[i] += changeOfVariableOffset;
   }
   return realSolutions;
}

UOSEProjectileFunctionLibrary::FPolynomialSolutions UOSEProjectileFunctionLibrary::_SolveBiquadratic(float coef4, float coef2, float coef0)
{
   FPolynomialSolutions quadraticSolutions = _SolveQuadratic(coef4, coef2, coef0);
   FPolynomialSolutions returnSolutions;
   for (int i = quadraticSolutions.Num() - 1; i >= 0; i--)
   {
      if (FMath::IsNearlyZero(quadraticSolutions[i]))
      {
         returnSolutions.Add(0.0f);
      }
      else if (quadraticSolutions[i] > 0.0f)
      {
         float positiveSolution = FMath::Sqrt(quadraticSolutions[i]);
         returnSolutions.Add(-positiveSolution);
         returnSolutions.Add(positiveSolution);
      }
   }
   return returnSolutions;
}

UOSEProjectileFunctionLibrary::FPolynomialSolutions UOSEProjectileFunctionLibrary::_SolveDepressedQuartic(float coef2, float coef1, float coef0)
{
   // Check for biquadratic case
   if (FMath::IsNearlyZero(coef1))
   {
      return _SolveBiquadratic(1.0f, coef2, coef0);
   }

   // Check for secretly cubic case
   if (FMath::IsNearlyZero(coef0))
   {
      return _SolveDepressedCubic(coef2, coef1);
   }

   // Use Ludovico Ferrari's method to find a sleight of hand that turns the equation
   // into an equality of two perfect squares. This involves finding the root of a
   // particular cubic equation, so that a key part of the expression will go to zero:
   float ferrariCoef2 = 2.5f * coef2;
   float ferrariCoef1 = 2.0f * coef2 * coef2 - coef0;
   float ferrariCoef0 = (coef2 * 0.5f) * ((coef2 * coef2) - coef0) - 0.5f * (0.5f * coef1) * (0.5f * coef1);

   // This cubic will have at least one and up to 3 real roots, but we want one big
   // enough to avoid a complex root later. A root of such bigness is theoretically
   // guaranteed.
   FPolynomialSolutions ferrariOffsets = _SolveCubic(1.0f, ferrariCoef2, ferrariCoef1, ferrariCoef0);
   float chosenFerrariOffset = 0.0f;
   float threshold = -0.5f * coef2;

   for (int i = 0; i < ferrariOffsets.Num(); i++)
   {
      if (FMath::IsFinite(ferrariOffsets[i]) && ferrariOffsets[i] > threshold)
      {
         chosenFerrariOffset = ferrariOffsets[i];
         break;
      }
   }

   // check against the thing that theoretically can't happen, but practically maybe could.
   if (!FMath::IsFinite(chosenFerrariOffset) || FMath::IsNaN(chosenFerrariOffset) || (chosenFerrariOffset <= threshold))
   {
      // Well, hell if I know
      FPolynomialSolutions noSolutions;
      return noSolutions;
   }

   float radical = FMath::Sqrt(coef2 + 2.0f * chosenFerrariOffset);

   float differenceCandidateOne = -(3.0f * coef2 + 2.0f * chosenFerrariOffset + 2.0f * coef1 / radical);
   float differenceCandidateTwo = -(3.0f * coef2 + 2.0f * chosenFerrariOffset - 2.0f * coef1 / radical);
   FPolynomialSolutions returnSolutions;

   if (differenceCandidateOne >= 0.0f)
   {
      returnSolutions.Add((radical + FMath::Sqrt(differenceCandidateOne)) * 0.5f);
      if (!FMath::IsNearlyZero(differenceCandidateOne))
      {
         returnSolutions.Add((radical - FMath::Sqrt(differenceCandidateOne)) * 0.5f);
      }
   }
   if (differenceCandidateTwo >= 0.0f)
   {
      returnSolutions.Add((-radical + FMath::Sqrt(differenceCandidateTwo)) * 0.5f);
      if (!FMath::IsNearlyZero(differenceCandidateTwo))
      {
         returnSolutions.Add((-radical - FMath::Sqrt(differenceCandidateTwo)) * 0.5f);
      }
   }

   return returnSolutions;
}


