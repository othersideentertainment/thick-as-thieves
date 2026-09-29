// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "Kismet/KismetSystemLibrary.h"

#include "OSEProjectileFunctionLibraryTypes.generated.h"

// Input parameters to PredictProjectilePath functions.
USTRUCT(BlueprintType)
struct FOSEPredictProjectilePathParams
{
   GENERATED_BODY();

   // Location of the start of the trace.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PredictProjectilePathParams)
   FVector StartLocation;

   // Initial launch velocity at the start of the trace.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PredictProjectilePathParams)
   FVector LaunchVelocity;

   // Whether to trace along the path looking for blocking collision and stopping at the first hit.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PredictProjectilePathParams)
   bool bTraceWithCollision;

   // Projectile radius, used when tracing for collision. If <= 0, a line trace is used instead.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PredictProjectilePathParams)
   float ProjectileRadius;

   // Maximum simulation time for the virtual projectile.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PredictProjectilePathParams)
   float MaxSimTime;

   // Whether or not to use TraceProfile, if tracing with collision.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PredictProjectilePathParams)
   bool bTraceWithProfile;

   // Trace channel to use, if tracing with collision.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PredictProjectilePathParams)
   FName TraceProfile;

   // Actors to ignore when tracing with collision.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PredictProjectilePathParams, AdvancedDisplay)
   TArray<TObjectPtr<AActor>> ActorsToIgnore;

   // Determines size of each sub-step in the simulation (chopping up MaxSimTime). Recommended between 10 to 30 depending on desired quality versus performance.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PredictProjectilePathParams, AdvancedDisplay)
   float SimFrequency;

   // Optional override of Gravity (if 0, uses WorldGravityZ).
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PredictProjectilePathParams, AdvancedDisplay)
   float OverrideGravityZ;

   // Debug drawing duration option.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PredictProjectilePathParams, AdvancedDisplay)
   TEnumAsByte<EDrawDebugTrace::Type> DrawDebugType;

   // Duration of debug lines (only relevant for DrawDebugType::Duration)
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PredictProjectilePathParams, AdvancedDisplay)
   float DrawDebugTime;

   // Trace against complex collision (triangles rather than simple primitives) if tracing with collision.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PredictProjectilePathParams, AdvancedDisplay)
   bool bTraceComplex;

   // Empty constructor. You typically want to use another one that enforces thought about reasonable values for the most important parameters.
   FOSEPredictProjectilePathParams()
   {
      Init(0.f, FVector::ZeroVector, FVector::ForwardVector, 1.f, false);
   }

   // Constructor defaulting to no collision.
   FOSEPredictProjectilePathParams(float InProjectileRadius, FVector InStartLocation, FVector InLaunchVelocity, float InMaxSimTime)
   {
      Init(InProjectileRadius, InStartLocation, InLaunchVelocity, InMaxSimTime, false);
   }

private:

   void Init(float InProjectileRadius, FVector InStartLocation, FVector InLaunchVelocity, float InMaxSimTime, bool InTraceWithCollision)
   {
      StartLocation = InStartLocation;
      LaunchVelocity = InLaunchVelocity;
      bTraceWithCollision = InTraceWithCollision;
      ProjectileRadius = InProjectileRadius;
      MaxSimTime = InMaxSimTime;
      SimFrequency = 20.f;
      OverrideGravityZ = 0.f;
      DrawDebugTime = 1.f;
      DrawDebugType = EDrawDebugTrace::None;
      bTraceComplex = false;
      bTraceWithProfile = false;
   }
};
