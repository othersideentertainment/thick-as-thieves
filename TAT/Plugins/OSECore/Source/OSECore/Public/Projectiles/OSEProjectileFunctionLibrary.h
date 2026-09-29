// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#include "OSEProjectileFunctionLibrary.generated.h"


struct FOSEPredictProjectilePathParams;
struct FPredictProjectilePathResult;

UCLASS()
class OSECORE_API UOSEProjectileFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   // Predict the arc of a virtual projectile affected by gravity with collision checks along the arc.
   // Returns true if it hit something.
   // 
   // Modified from GameplayStatics version to support collision profiles
   static bool PredictProjectilePath(const UObject* worldContextObject, const FOSEPredictProjectilePathParams& predictParams, FPredictProjectilePathResult& predictResult);

   // Predict the arc of a virtual projectile affected by gravity with collision checks along the arc. Returns a list of positions of the simulated arc and the destination reached by the simulation.
   // Returns true if it hit something (if tracing with collision).
   // @param outPathPositions       Predicted projectile path.Ordered series of positions from StartPos to the end.Includes location at point of impact if it hit something.
   // @param outHit                 Predicted hit result, if the projectile will hit something
   // @param outLastTraceDestination   Goal position of the final trace it did. Will not be in the path if there is a hit.
   // @param startPos               First start trace location
   // @param launchVelocity         Velocity the "virtual projectile" is launched at
   // @param tracePath              Trace along the entire path to look for blocking hits
   // @param projectileRadius       Radius of the virtual projectile to sweep against the environment
   // @param traceProfile           TraceProfile to trace against, if bTracePath is true.
   // @param traceComplex           Use TraceComplex(trace against triangles not primitives)
   // @param actorsToIgnore         Actors to exclude from the traces
   // @param drawDebugType          Debug type(one - frame, duration, persistent)
   // @param drawDebugTime          Duration of debug lines(only relevant for DrawDebugType::Duration)
   // @param simFrequency           Determines size of each sub - step in the simulation(chopping up MaxSimTime)
   // @param maxSimTime             Maximum simulation time for the virtual projectile.
   // @param overrideGravityZ       Optional override of Gravity(if 0, uses WorldGravityZ)
   // @return                    True if hit something along the path(if tracing with collision).
   UFUNCTION(BlueprintCallable, Category = "Game|OSE", DisplayName = "Predict Projectile Path By Profile", meta = (WorldContext = "worldContextObject", AutoCreateRefTerm = "actorsToIgnore", AdvancedDisplay = "drawDebugTime, drawDebugType, simFrequency, maxSimTime, overrideGravityZ", TraceChannel = ECC_WorldDynamic, bTracePath = true))
   static bool Blueprint_PredictProjectilePath_ByTraceProfile(const UObject* worldContextObject, FHitResult& outHit, TArray<FVector>& outPathPositions, FVector& outLastTraceDestination, FVector startPos, FVector launchVelocity, bool tracePath, float projectileRadius, FName traceProfile, bool traceComplex, const TArray<AActor*>& actorsToIgnore, EDrawDebugTrace::Type drawDebugType, float drawDebugTime, float simFrequency = 15.f, float maxSimTime = 2.f, float overrideGravityZ = 0);

   /**
    * Plot a course (direction vector) from Source to Target positions, given a moving target's veloicty and an interceptor's available speed.
    *
    * @return                    True if an intercept course is possible at the given speed
    * @param SourcePoint         Where intercepter would start (in any distance units)
    * @param InterceptorSpeed    How fast interceptor can travel (in distance units/second)
    * @param TargetPosition      Where the target is at the start of interceptor's travel (any distance units)
    * @param TargetVelocity      Target's velocity (any distance units/second)
    * @param Course              Direction interceptor should travel to overtake target, if intercept is possible. Otherwise, a direction that generally leads the target, but is not expected to intercept.
    */
   UFUNCTION(BlueprintPure, Category = "Math|Vector|OSE")
   static bool PlotInterceptCourse(FVector sourcePoint, float interceptorSpeed, FVector targetPosition, FVector targetVelocity, FVector& course);

   static float PlotInterceptTime(FVector sourcePoint, float interceptorSpeed, FVector targetPosition, FVector targetVelocity, FVector targetAcceleration = FVector::ZeroVector);

   /* Given an Actor class, presumed to have a Projectile Movement Component, return that projectile's speed (or a negative value if failed) */
   UFUNCTION(BlueprintPure, Category = "Projectile")
   static float GetProjectileSpeedByClass(UClass* projectileClass);

   /* Given an Actor class, presumed to have a Projectile Movement Component, return that component (or null if failed) */
   UFUNCTION(BlueprintPure, Category = "Projectile")
   static UProjectileMovementComponent* GetProjectileMovementByClass(UClass* projectileClass);

private:

   /* @TODO: There is very similar polynomial root-finding code in the gte::RootsPolynomial
   *  class, so it may be worth using that instead. Not 100% sure if it's suitable or not. -TJS
   */

   /* We use the same declaration for stack-allocated arrays of solutions for all of the
   * polynominal-root functions, although we'll only need as many as four roots for
   * Quartic equations (and not necessarily even then). But this lets us pass these
   * freely between those functions, for example in the case where a cubic equation
   * turns out to really be a quadratic in disguise. Besides, not too worried about
   * allocating an extra float or two on the stack.
   */
   typedef TArray<float, TFixedAllocator<4>> FPolynomialSolutions;

   /**
   * Find the real solutions (if any) to a quadratic equation in standard ax^2 + bx + c form.
   *
   * @param a                       Coefficient of the quadratic term
   * @param b                       Coefficient of the linear term
   * @param c                       Coefficient of the constant term
   * @returns                       real roots, if any
   */
   static FPolynomialSolutions _SolveQuadratic(float a, float b, float c);

   /**
   * Find the real solutions to a given cubic equation.
   *
   * @param coef3                   Coefficient of the x^3 term
   * @param coef2                   Coefficient of the x^2 term
   * @param coef1                   Coefficient of the x term
   * @param coef0                   Coefficient of the constant term
   * @returns                       real roots, if any
   */
   static FPolynomialSolutions _SolveCubic(float coef3, float coef2, float coef1, float coef0);

   /**
   * Find the real solutions to a given cubic equation in "depressed" form, i.e. with
   * coefficient equal to 1 on the cubic term and 0 on the quadratic term. The remaining
   * coefficients have conventional names, used here.
   *
   * @param p                       Coefficient of the x term
   * @param q                       Coefficient of the constant term
   * @returns                       real roots
   */
   static FPolynomialSolutions _SolveDepressedCubic(float p, float q);

   /**
   * Find the real solutions to a given quartic equation.
   *
   * @param coef4                   Coefficient of the x^4 term
   * @param coef3                   Coefficient of the x^3 term
   * @param coef2                   Coefficient of the x^2 term
   * @param coef1                   Coefficient of the x term
   * @param coef0                   Coefficient of the constant term
   * @returns                       real roots, if any
   */
   static FPolynomialSolutions _SolveQuartic(float coef4, float coef3, float coef2, float coef1, float coef0);

   /**
   * Find the real solutions to a given quartic equation in "depressed" form, i.e. with
   * coefficent equal to 1 on the x^4 term and 0 on the x^3 term.
   *
   * @param coef4                   Coefficient of the x^4 term
   * @param coef3                   Coefficient of the x^3 term
   * @param coef2                   Coefficient of the x^2 term
   * @param coef1                   Coefficient of the x term
   * @param coef0                   Coefficient of the constant term
   * @returns                       real roots, if any
   */
   static FPolynomialSolutions _SolveDepressedQuartic(float coef2, float coef1, float coef0);

   /**
   * Find the cube root of the input. Safe for negative inputs (unlike FMath::Pow)
   */
   static float _CubeRoot(float base) { return ((base < 0.0f) ? -FMath::Pow(-base, 1.0f / 3.0f) : FMath::Pow(base, 1.0f / 3.0f)); };

   /**
   * Find the real solutions to a given biquadratic equation (i.e. a quartic equation with
   * odd-numbered coefficients equal to 0).
   *
   * @param coef4                   Coefficient of the x^4 term
   * @param coef2                   Coefficient of the x^2 term
   * @param coef0                   Coefficient of the constant term
   * @returns                       real roots, if any
   */
   static FPolynomialSolutions _SolveBiquadratic(float coef4, float coef2, float coef0);
};
