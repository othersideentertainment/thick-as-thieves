// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Environment/TATPuddleTypes.h"

// ose
#include "OSECoreCheats.h"

// ue
#include "CoreMinimal.h"

#include "TATPuddleUtilities.generated.h"

#if ENABLE_DRAW_DEBUG && OSE_CHEATS_ENABLED
#define TAT_ALLOW_PUDDLE_DEBUG 1
#else
#define TAT_ALLOW_PUDDLE_DEBUG 0
#endif

namespace PuddleHelpers
{
   enum class EPuddleDebugDraw : uint8
   {
      Hidden = 0,
      HealthBars = 1 << 0,
      StateIndicator = 1 << 1,
      OutsideTraces = 1 << 2,
      Radius = 1 << 3,
      BoundingBox = 1 << 4,
      Text = 1 << 5,
   };
   ENUM_CLASS_FLAGS(EPuddleDebugDraw);

#if TAT_ALLOW_PUDDLE_DEBUG
   bool IsPuddleDebugEnabled();
   EPuddleDebugDraw GetPuddleDebugDraw();
#else
   FORCEINLINE constexpr bool IsPuddleDebugEnabled() { return false; }
   FORCEINLINE constexpr EPuddleDebugDraw GetPuddleDebugDraw() { return EPuddleDebugDraw::Hidden; }
#endif

   struct FVectorAverage
   {
      FVector Sum = FVector::ZeroVector;
      float Weight = 0.0f;
      FORCEINLINE void Add(const FVector& d, float weight = 1.0f) { Sum += d * weight; Weight += weight; };
      FORCEINLINE FVector Get() const { check(Weight > 0); return (Sum / Weight).GetSafeNormal(); }
      FORCEINLINE FVector GetOr(const FVector& defaultValue) const { return (Weight > 0) ? (Sum / Weight).GetSafeNormal() : defaultValue; }
      FORCEINLINE explicit operator bool() const { return Weight > 0; }
   };

   FORCEINLINE FVector PositionOnOval(const FVector& origin, const FVector2D& extent, const FQuat& orientation, float angleRad, float vertOffset = 0.0f)
   {
      return origin
         + (orientation.GetForwardVector() * (extent.X * FMath::Cos(angleRad)))
         + (orientation.GetRightVector() * (extent.Y * FMath::Sin(angleRad)))
         + (orientation.GetUpVector() * vertOffset);
   }

   FORCEINLINE float DistanceBetweenSpheres(const FSphere& a, const FSphere& b)
   {
      const float centerDistSquared = FVector::DistSquared(a.Center, b.Center);
      const float radiusSum = FMath::Max(0.0f, a.W + b.W);
      if (centerDistSquared <= FMath::Square(radiusSum))
      {
         return 0.0f;
      }
      // Can't avoid the sqrt here because we need to subtract the sum of the sphere's radii
      return FMath::Sqrt(centerDistSquared) - radiusSum;
   }

   void DrawDebugOval(const UWorld* world, const FVector& origin, const FVector2D& extent, const FQuat& orientation, int32 numSegments, const FColor& color,
      bool persistentLines = false, float lifeTime = -1.0f, uint8 depthPriority = 0, float thickness = 0.0f);

   void DrawDebugRadialProgressBar(const UWorld* world, const FVector& origin, const FQuat& orientation, float normalizedValue,
      const FColor& emptyColor, float emptyThickness, const FColor& valueColor, float valueThickness, int32 numSegments, float radius,
      bool persistentLines = false, float lifeTime = -1.0f, uint8 depthPriority = 0);

   void DrawDebugDirectionAndVelocity(const UWorld* world, const FVector& pos, const FVector& dir, float velocity,
      bool persistentLines = false, float lifeTime = -1.0f, uint8 depthPriority = 0, float thickness = 0.0f);

   void DrawDebugCloudIcon(const UWorld* world, const FVector& position, float size, const FColor& cloudColor, const FColor& rainColor, bool raindropsAsQuads = false,
      const TOptional<FQuat>& orientation = NullOpt, bool persistent = false, float lifeTime = -1.0f, uint8 depthPriority = 0, float thickness = 0.0f);
}

USTRUCT(BlueprintType)
struct TAT_API FTATLiquidSimulationParams
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   int32 ParticleCount = 10;

   /// If greater than zero, particles that move farther away than this are marked as dead (no longer simulated)
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   float MaxRadius = 0.0f;

   /// If greater than zero, particles that move more than this distance under the simulation's origin are marked as dead (no longer simulated)
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   float MaxHeight = 0.0f;

   /// Number of iterations to run the simulation.
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   int32 IterationCount = 20;

   /// The virtual time (not game time!) over which to simulate the particles.
   /// The simulation's time step is the runtime divided by the iteration count.
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   float Runtime = 2.0f;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   float GravityMultiplier = 1.0f;

   FORCEINLINE float GetTimeStep() const { return FMath::Max(0.01f, Runtime) / static_cast<float>(FMath::Max(1, IterationCount)); }
};

USTRUCT(BlueprintType)
struct TAT_API FTATPuddleProjectionTraceParams
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FFloatInterval PuddleThicknessRange = FFloatInterval(40.0f, 80.0f);

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FInt32Interval TraceCount = FInt32Interval(5, 8);

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   float TraceConeAngleDeg = 45.0f;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   TEnumAsByte<ECollisionChannel> CollisionChannel = ECC_WorldStatic;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   bool TraceComplex = false;
};

namespace PuddleHelpers
{
   struct FLiquidParticle
   {
      FVector PrevPosition = FVector::ZeroVector;
      FVector Position = FVector::ZeroVector;
      FVector Velocity = FVector::ZeroVector;
      FVector HitNormal = FVector::ZeroVector;
      TWeakObjectPtr<AActor> HitActor;
      bool Hit = false;
      bool Dead = false;

      FORCEINLINE void Init(const FVector& position, const FVector& velocity)
      {
         PrevPosition = position;
         Position = position;
         Velocity = velocity;
         HitNormal = FVector::ZeroVector;
         HitActor = nullptr;
         Hit = false;
         Dead = false;
      }

      FORCEINLINE void MarkAsHit(const FHitResult& hitResult)
      {
         Hit = true;
         Position = hitResult.Location;
         HitNormal = hitResult.Normal;
         HitActor = hitResult.GetActor();
      }

      FORCEINLINE void MarkAsDead() { Dead = true; }
      FORCEINLINE bool IsAlive() const { return !Hit && !Dead; }
   };

   class FLiquidSimulation
   {
   public:
      using FCollisionCallback = TFunction<void(int32, const FHitResult&)>;

   private:
      TWeakObjectPtr<UWorld> _world;
      FTATLiquidSimulationParams _simParams;
      TArrayView<FLiquidParticle> _particles;
      FVector _gravity = FVector(0.0f, 0.0f, -981.0f);
      FVector _origin = FVector::ZeroVector;
      ECollisionChannel _collisionChannel = ECC_WorldStatic;
      bool _drawDebug = false;
      float _drawDebugDuration = 2.0f;
      FCollisionCallback _collisionCallback;

      int32 _traceCount = 0;
      FHitResult _hitResult{};
      FCollisionQueryParams _queryParams{};
      FCollisionResponseParams _responseParams{};

   public:
      FLiquidSimulation(UWorld* world, const FTATLiquidSimulationParams& simParams, TArrayView<FLiquidParticle> particles, const FVector& gravity, const FVector& origin,
         ECollisionChannel collisionChannel = ECC_WorldStatic, bool drawDebug = false, float drawDebugDuration = 2.0f);

      void SetCollisionCallback(const FCollisionCallback& callback) { _collisionCallback = callback; }

      /// Runs one iteration of the simulation, returning the number of particles still active
      int32 Update(float deltaTime);

      /// Runs all iterations of the simulation
      void RunSimulation();

      /// Returns true if no particles are still active
      bool IsFinished() const;

      /// Does a simulation bounds check on a world location
      bool IsLocationInSimulationBounds(const FVector& pos) const;

      /// Expose access to line trace config
      FORCEINLINE FCollisionQueryParams& GetCollisionQueryParamsRef() { return _queryParams; }
      FORCEINLINE FCollisionResponseParams& GetCollisionResponseParamsRef() { return _responseParams; }

      /// Returns the number of traces performed (just useful for stats and debug drawing)
      FORCEINLINE int32 GetTraceCount() const { return _traceCount; }

   private:
      void _DrawDebugParticleMove(const FVector& start, const FVector& end, const FColor& color) const;
      const FHitResult* _LineTrace(const FVector& start, const FVector& end);
   };
}

/// Return value for UTATPuddleUtilities::RunLiquidParticleExplosionSimulation
USTRUCT(BlueprintType)
struct TAT_API FTATLiquidExplosionResult
{
   GENERATED_BODY()
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FVector Location = FVector::ZeroVector;
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FVector Velocity = FVector::ZeroVector;
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FVector HitNormal = FVector::ZeroVector;
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   TObjectPtr<AActor> HitActor;
};

UCLASS(BlueprintType)
class TAT_API UTATPuddleUtilities : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   static bool IsPuddleOutside(const UObject* contextObject, const FTATPuddle& puddle, AActor* actorToIgnore, bool debugDraw = false, float debugDrawLifetime = 2.0f);

   UFUNCTION(BlueprintPure, Category = "Puddle Utilities")
   static FORCEINLINE bool IsPuddleDebugEnabled() { return PuddleHelpers::IsPuddleDebugEnabled(); }

   UFUNCTION(BlueprintPure, Category = "Puddle Utilities")
   static bool TargetMatchesPuddleFilter(const AActor* actor, ETATPuddleTargetFilter targetFilter);

   UFUNCTION(BlueprintPure, Category = "Puddle Utilities")
   static FVector GetPositionAroundPuddle(const FVector& location, const FVector& extent, const FRotator& rotation, float angleDeg, float extentMultiplier = 0.8f);

   UFUNCTION(BlueprintPure, Category = "Puddle Utilities")
   static ETATPuddleSurfaceAngle GetPuddleSurfaceAngle(const FRotator& rotation);

   /// Does several traces to compute the transform of a puddle in the world.
   /// @param seed The seed to use when generating random values. Set to zero for a random seed.
   /// @param impactLocation The location to spawn the puddle
   /// @param impactDirection The direction the puddle was spawned from (eg. the projectile's velocity)
   /// @param puddleRadius The radius of the puddle to generate
   /// @return True if a puddle can be spawned here, otherwise false if no valid location could be computed.
   UFUNCTION(BlueprintCallable, Category = "Puddle Utilities", Meta = (WorldContext = "contextObject"))
   static bool PuddleProjectionTrace(
      const UObject* contextObject,
      FTATPuddleTransform& puddleTransform,
      int32 seed,
      const FVector& impactLocation,
      const FVector& impactDirection,
      float puddleRadius,
      const FTATPuddleProjectionTraceParams& projectionTraceParams,
      const AActor* ignoreActor = nullptr,
      bool drawDebug = false,
      float drawDebugDuration = 2.0f);

   /// Runs a light liquid particle simulation and returns the locations and surface normals of any particles that hit something.
   /// @param seed The seed to use when generating random values. Set to zero for a random seed.
   /// @param location Simulation origin point
   /// @param direction Used for computing the initial location and velocity of particles.
   /// @param initialLocationOffset Push out the initial particle locations this far from the origin.
   /// @param initialDirectionalVelocity How much initial particle velocity to apply from the direction argument.
   /// @param initialRandomVelocity How much initial particle velocity to apply from a random direction vector.
   /// @param initialVelocityRandomConeAngleDeg If greater than zero, limits the random direction vector to within this angle of the direction argument.
   UFUNCTION(BlueprintCallable, Category = "Puddle Utilities", Meta = (WorldContext = "contextObject"))
   static bool RunLiquidParticleExplosionSimulation(
      const UObject* contextObject,
      TArray<FTATLiquidExplosionResult>& particleHits,
      int32 seed,
      const FVector& location,
      const FVector& direction,
      const FFloatInterval& initialLocationOffset,
      const FFloatInterval& initialDirectionalVelocity,
      const FFloatInterval& initialRandomVelocity,
      float initialVelocityRandomConeAngleDeg = 0.0f,
      const FTATLiquidSimulationParams& simulationParams = FTATLiquidSimulationParams(),
      TEnumAsByte<ECollisionChannel> collisionChannel = ECC_WorldStatic,
      bool drawDebug = false,
      float drawDebugDuration = 2.0f);

   /// Generates a puddle, then runs a light liquid particle simulation to find nearby surfaces to spawn more puddles.
   /// @param seed The seed to use when generating random values. Set to zero for a random seed.
   /// @param primaryImpactPuddleRadius The radius of the primary puddle that is spawned at the impact location
   /// @param secondaryImpactPuddleRadius The radius of secondary puddles spawned near the primary one
   /// @param maxPuddleRadius When secondary puddles would otherwise be spawned too close to the primary one, the primary puddle's radius is expanded instead, up to this maximum radius.
   /// @param explosionForce Velocity range of the simulated liquid particles. This determines how far away the secondary puddles are spawned.
   /// @param skipPrimaryPuddle Don't include the primary puddle in the return values
   UFUNCTION(BlueprintCallable, Category = "Puddle Utilities", Meta = (WorldContext = "contextObject"))
   static bool SimulatePuddleExplosion(
      const UObject* contextObject,
      TArray<FTATPuddleTransform>& puddleTransforms,
      int32 seed,
      const FVector& impactLocation,
      const FVector& impactVelocity,
      float primaryImpactPuddleRadius,
      float secondaryImpactPuddleRadius,
      float maxPuddleRadius,
      const FFloatInterval& explosionForce,
      const FTATLiquidSimulationParams& simulationParams,
      const FTATPuddleProjectionTraceParams& projectionTraceParams,
      const AActor* ignoreActor = nullptr,
      bool skipPrimaryPuddle = false,
      bool drawDebugPuddleProjection = false,
      bool drawDebugLiquidSimulation = false,
      float drawDebugDuration = 2.0f);

};
