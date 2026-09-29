// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Environment/TATPuddleUtilities.h"

// tat
#include "Environment/TATWeatherUtilities.h"

// ue
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPuddleUtilities)

DEFINE_LOG_CATEGORY_STATIC(LogTATPuddleUtilities, Log, All);

#if TAT_ALLOW_PUDDLE_DEBUG
static TAutoConsoleVariable<int32> CVarTATPuddleDebug(
    TEXT("tat.puddle.debug"),
    0,
    TEXT("Show puddle debug info if value is greater than 1. Higher values show more info.\n")
    TEXT("0: Hidden\n")
    TEXT("1: Show health bars\n")
    TEXT("2: Show health bars, outside indicator, and radius\n")
    TEXT("3: Show health bars, outside indicator, and collider box\n")
    TEXT("4: Show health bars, outside indicator, collider box, and text\n"),
    ECVF_Default);
#endif

namespace PuddleHelpers
{
#if TAT_ALLOW_PUDDLE_DEBUG
   bool IsPuddleDebugEnabled()
   {
      return CVarTATPuddleDebug.GetValueOnGameThread() > 0;
   }
   EPuddleDebugDraw GetPuddleDebugDraw()
   {
      constexpr int32 maxValue = 4;
      switch (FMath::Clamp(CVarTATPuddleDebug.GetValueOnGameThread(), 0, maxValue))
      {
      case 1:
         return EPuddleDebugDraw::HealthBars;
      case 2:
         return EPuddleDebugDraw::HealthBars | EPuddleDebugDraw::StateIndicator | EPuddleDebugDraw::Radius;
      case 3:
         return EPuddleDebugDraw::HealthBars | EPuddleDebugDraw::StateIndicator | EPuddleDebugDraw::BoundingBox;
      case 4:
         return EPuddleDebugDraw::HealthBars | EPuddleDebugDraw::StateIndicator | EPuddleDebugDraw::OutsideTraces | EPuddleDebugDraw::BoundingBox | EPuddleDebugDraw::Text;
      default:
         break;
      }
      return EPuddleDebugDraw::Hidden;
   }
#endif

   void DrawDebugOval(const UWorld* world, const FVector& origin, const FVector2D& extent, const FQuat& orientation, int32 numSegments, const FColor& color,
      bool persistentLines, float lifeTime, uint8 depthPriority , float thickness)
   {
      const float angleOffsetRad = (UE_PI * 2.0f) / static_cast<float>(numSegments);
      for (int32 i = 0; i <= numSegments; i++)
      {
         const FVector start = PositionOnOval(origin, extent, orientation, static_cast<float>(i) * angleOffsetRad);
         const FVector end = PositionOnOval(origin, extent, orientation, static_cast<float>(i + 1) * angleOffsetRad);
         DrawDebugLine(world, start, end, color, persistentLines, lifeTime, depthPriority, thickness);
      }
   }

   void DrawDebugRadialProgressBar(const UWorld* world, const FVector& origin, const FQuat& orientation, float normalizedValue,
      const FColor& emptyColor, float emptyThickness, const FColor& valueColor, float valueThickness, int32 numSegments, float radius,
      bool persistentLines, float lifeTime, uint8 depthPriority)
   {
      numSegments = FMath::Max(3, numSegments);
      normalizedValue = FMath::Clamp(normalizedValue, 0.0f, 1.0f);

      constexpr float padding = 3.0f; // spacing between the "empty" lines and the "value" lines
      const float minRadius = (emptyThickness * 2) + (padding * 2) + valueThickness + 1.0f;
      radius = FMath::Max(radius, minRadius);

      const float emptyRadiusOuter = radius;
      const float valueRadius = emptyRadiusOuter - padding - (FMath::Max(1.0f, valueThickness) * 0.5f);
      const float emptyRadiusInner = valueRadius - padding;

      auto getAngleRad = [](int32 idx, int32 total, float maxPercent)
      {
         static constexpr float fullRotationRad = UE_PI * 2.0f;
         static constexpr float offsetRad = fullRotationRad * 0.25f;
         const float angleIncrementRad = (fullRotationRad * maxPercent) / static_cast<float>(total);
         return (static_cast<float>(idx) * angleIncrementRad) + offsetRad;
      };

      if (emptyColor.A > 0)
      {
         for (int32 i = 1; i <= numSegments; i++)
         {
            const FVector outerPtA = PositionOnOval(origin, FVector2D(emptyRadiusOuter), orientation, getAngleRad(i - 1, numSegments, 1.0f));
            const FVector outerPtB = PositionOnOval(origin, FVector2D(emptyRadiusOuter), orientation, getAngleRad(i, numSegments, 1.0f));
            DrawDebugLine(world, outerPtA, outerPtB, emptyColor, persistentLines, lifeTime, depthPriority, emptyThickness);

            const FVector innerPtA = PositionOnOval(origin, FVector2D(emptyRadiusInner), orientation, getAngleRad(i - 1, numSegments, 1.0f));
            const FVector innerPtB = PositionOnOval(origin, FVector2D(emptyRadiusInner), orientation, getAngleRad(i, numSegments, 1.0f));
            DrawDebugLine(world, innerPtA, innerPtB, emptyColor, persistentLines, lifeTime, depthPriority, emptyThickness);
         }
      }

      if (normalizedValue > 0 && valueColor.A > 0)
      {
         const float vertOffset = valueThickness;
         for (int32 i = 1; i <= numSegments; i++)
         {
            const FVector ptA = PositionOnOval(origin, FVector2D(valueRadius), orientation, getAngleRad(i - 1, numSegments, normalizedValue), vertOffset);
            const FVector ptB = PositionOnOval(origin, FVector2D(valueRadius), orientation, getAngleRad(i, numSegments, normalizedValue), vertOffset);
            DrawDebugLine(world, ptA, ptB, valueColor, persistentLines, lifeTime, depthPriority, valueThickness);
         }
      }
   }

   void DrawDebugDirectionAndVelocity(const UWorld* world, const FVector& pos, const FVector& dir, float velocity,
      bool persistentLines, float lifeTime, uint8 depthPriority, float thickness)
   {
      if (dir.IsNearlyZero())
      {
         DrawDebugPoint(world, pos, 2.0f, FColor(200, 200, 200), persistentLines, lifeTime, depthPriority);
         return;
      }
      const FVector2D velocityRange = FVector2D(50.0f, 1000.0f);
      const float lineLength = FMath::GetMappedRangeValueClamped(velocityRange, FVector2D(5.0f, 25.0f), velocity);
      const uint8 lineVal = static_cast<uint8>(FMath::GetMappedRangeValueClamped(velocityRange, FVector2D(100.0f, 255.0f), velocity));
      const uint8 lineHue = static_cast<uint8>(FMath::GetMappedRangeValueClamped(velocityRange, FVector2D(0.0f, 100.0f), velocity));
      DrawDebugLine(world, pos, pos + (dir.GetSafeNormal() * lineLength), FLinearColor::MakeFromHSV8(lineHue, 225, lineVal).ToFColorSRGB(),
         persistentLines, lifeTime, depthPriority, thickness);
   }

   void DrawDebugCloudIcon(const UWorld* world, const FVector& position, float size, const FColor& cloudColor, const FColor& rainColor, bool raindropsAsQuads,
      const TOptional<FQuat>& orientation, bool persistent, float lifeTime, uint8 depthPriority, float thickness)
   {
      auto transform = [size, orientation](const FVector2D& v, const FVector& vertSpaceOffset) -> FVector
      {
         const FVector result = (FVector(0, v.X - 0.5, v.Y - 0.5) + vertSpaceOffset) * size;
         return orientation ? orientation->RotateVector(result) : result;
      };

      auto drawOutline = [&](TConstArrayView<FVector2D> verts, const FVector& vertSpaceOffset, const FColor& color)
      {
         for (int32 i = 1; i <= verts.Num(); i++)
         {
            const FVector a = position + transform(verts[i - 1], vertSpaceOffset);
            const FVector b = position + transform(verts[i % verts.Num()], vertSpaceOffset);
            DrawDebugLine(world, a, b, color, persistent, lifeTime, depthPriority, thickness);
         }
      };

      auto drawQuad = [&](TConstArrayView<FVector2D> verts, const FVector& vertSpaceOffset, const FColor& color)
      {
         check(verts.Num() == 4);
         // DrawDebugMesh requires a regular TArray<FVector>, so it's not possible to avoid an allocation here
         const TArray<FVector> vertices = {
            position + transform(verts[0], vertSpaceOffset),
            position + transform(verts[1], vertSpaceOffset),
            position + transform(verts[2], vertSpaceOffset),
            position + transform(verts[3], vertSpaceOffset),
         };
         static const TArray<int32> indices = { 0, 1, 2, 2, 3, 0 };
         DrawDebugMesh(world, vertices, indices, color, persistent, lifeTime, depthPriority);
      };

      // Draw cloud outline
      static const TArray<FVector2D, TInlineAllocator<11>> cloudVerts = {
         {1.0000,0.6239},{0.8780,0.7712},{0.5918,0.7803},{0.4754,0.9803},{0.2268,0.9122},{0.1428,0.7266},
         {0.0000,0.5588},{0.1397,0.3798},{0.4446,0.3663},{0.7942,0.4035},{0.9650,0.4668},
      };
      drawOutline(cloudVerts, FVector::Zero(), cloudColor);

      // Draw raindrop (either as quads or an outline)
      static const TArray<FVector2D, TInlineAllocator<4>> raindropVerts = {
         {0.5964,0.0800},{0.6386,0.1095},{0.4732,0.3455},{0.4315,0.3157},
      };
      const FVector raindropHoriz{ 0, 0.35, 0 };
      const FVector raindropVert{ 0, 0, -0.1 };
      if (raindropsAsQuads)
      {
         drawQuad(raindropVerts, raindropVert, rainColor);
         drawQuad(raindropVerts, raindropVert - raindropHoriz, rainColor);
         drawQuad(raindropVerts, raindropVert + raindropHoriz, rainColor);
      }
      else
      {
         drawOutline(raindropVerts, raindropVert, rainColor);
         drawOutline(raindropVerts, raindropVert - raindropHoriz, rainColor);
         drawOutline(raindropVerts, raindropVert + raindropHoriz, rainColor);
      }
   }

   FLiquidSimulation::FLiquidSimulation(UWorld* world, const FTATLiquidSimulationParams& simParams, TArrayView<FLiquidParticle> particles, const FVector& gravity,
      const FVector& origin, ECollisionChannel collisionChannel, bool drawDebug, float drawDebugDuration)
      : _world(world)
      , _simParams(simParams)
      , _particles(particles)
      , _gravity(gravity)
      , _origin(origin)
      , _collisionChannel(collisionChannel)
      , _drawDebug(drawDebug)
      , _drawDebugDuration(drawDebugDuration)
   {
   }

   int32 FLiquidSimulation::Update(float deltaTime)
   {
      int32 remainingParticleCount = 0;

      for (int32 particleIdx = 0; particleIdx < _particles.Num(); particleIdx++)
      {
         FLiquidParticle& p = _particles[particleIdx];
         if (!p.IsAlive())
         {
            continue;
         }

         p.PrevPosition = p.Position;

         p.Velocity += _gravity * _simParams.GravityMultiplier * deltaTime;

         const FVector newPos = p.Position + (p.Velocity * deltaTime);
         if (!IsLocationInSimulationBounds(newPos))
         {
            p.MarkAsDead();
            continue;
         }

         if (const FHitResult* hit = _LineTrace(p.PrevPosition, newPos))
         {
            _DrawDebugParticleMove(p.PrevPosition, hit->Location, FColor(210, 255, 210));
            p.MarkAsHit(*hit);
            if (_collisionCallback)
            {
               _collisionCallback(particleIdx, *hit);
            }
            continue;
         }

         _DrawDebugParticleMove(p.PrevPosition, newPos, FColor(230, 230, 255));
         p.Position = newPos;

         ++remainingParticleCount;
      }

      return remainingParticleCount;
   }

   void FLiquidSimulation::RunSimulation()
   {
      const float timeStep = _simParams.GetTimeStep();
      int32 particlesRemaining = _particles.Num();
      for (int32 iterIdx = 0; iterIdx < _simParams.IterationCount && particlesRemaining > 0; ++iterIdx)
      {
         particlesRemaining = Update(timeStep);
      }
   }

   bool FLiquidSimulation::IsFinished() const
   {
      for (const FLiquidParticle& p : _particles)
      {
         if (p.IsAlive())
         {
            return false;
         }
      }
      return true;
   }

   bool FLiquidSimulation::IsLocationInSimulationBounds(const FVector& pos) const
   {
      // If we have both a max radius and a max height, this uses a weird bounds shape - it's a rounded capsule at the top and a flat capsule at the bottom.
      // The max height only extends the capsule down, not up. This is primarily because we don't want particles flying too far in general,
      // but if we're simulating something high up, we want to give the particles some extra space to fall to the ground.

      // TODO: Make MaxHeight take gravity into account instead of just using world Z.
      // This isn't needed right now but could be handy if we want to support different gravity directions in the future.

      if (_simParams.MaxRadius > 0)
      {
         const float maxRadiusSquared = FMath::Square(_simParams.MaxRadius);

         // If we don't have a max height, or we do have a max height and the particle is above the origin point, use a 3d radius check
         if (_simParams.MaxHeight == 0 || pos.Z >= _origin.Z)
         {
            return FVector::DistSquared(pos, _origin) <= maxRadiusSquared;
         }

         // If the particle is below the origin point, just clamp on the X and Y axis (eg. clamp to a cylinder shape)
         if (FVector2D::DistSquared(FVector2D(pos), FVector2D(_origin)) > maxRadiusSquared)
         {
            return false;
         }
      }

      // Don't let the particle fall too far down on the Z axis
      if (_simParams.MaxHeight > 0 && _origin.Z - pos.Z > _simParams.MaxHeight)
      {
         return false;
      }

      return true;
   }

   void FLiquidSimulation::_DrawDebugParticleMove(const FVector& start, const FVector& end, const FColor& color) const
   {
      if (!_drawDebug || _world.Get() == nullptr)
      {
         return;
      }
      DrawDebugLine(_world.Get(), start, end, color, false, _drawDebugDuration);
   }

   const FHitResult* FLiquidSimulation::_LineTrace(const FVector& start, const FVector& end)
   {
      UWorld* world = _world.Get();
      if (world == nullptr)
      {
         return nullptr;
      }
      ++_traceCount;
      if (world->LineTraceSingleByChannel(_hitResult, start, end, _collisionChannel, _queryParams, _responseParams))
      {
         return &_hitResult;
      }
      return nullptr;
   }
}

// static
bool UTATPuddleUtilities::IsPuddleOutside(const UObject* contextObject, const FTATPuddle& puddle, AActor* actorToIgnore, bool debugDraw, float debugDrawLifetime)
{
   const UWorld* world = GEngine->GetWorldFromContextObject(contextObject, EGetWorldErrorMode::ReturnNull);
   if (world == nullptr)
   {
      return false;
   }

   constexpr bool persistentDebugLines = false;

   // Based on the X and Y extent of the puddle (whichever is larger), determine how many line traces we should do
   static const FVector2f extentTraceRange{ 50.0f, 500.0f };
   static const FVector2f extentTraceCount{ 3, 8 };

   auto isLocationOutside = [&](const FVector& pos) -> bool
   {
      const bool isOutside = !UTATWeatherUtilities::LineTraceCheckIfLocationIsInside(contextObject, pos, 0, actorToIgnore);
#if TAT_ALLOW_PUDDLE_DEBUG
      if (debugDraw)
      {
         const FColor color = isOutside ? FColor::Red : FColor::Green;
         const float thickness = isOutside ? 2.0f : 1.25f;
         DrawDebugLine(world, pos, pos + FVector(0, 0, 10000), color, persistentDebugLines, debugDrawLifetime, 0, thickness);
         DrawDebugLine(world, pos, puddle.Location, FColor::White, persistentDebugLines, debugDrawLifetime);
      }
#endif
      return isOutside;
   };

   // For small puddles, just do one trace in the center
   const float xyExtent = FMath::Max(puddle.Extent.X, puddle.Extent.Y);
   if (xyExtent < extentTraceRange.X)
   {
      return isLocationOutside(puddle.Location);
   }

   // For larger puddles, do a few traces around the whole puddle area
   int32 numTracesAroundPuddle = FMath::RoundToInt32(FMath::GetMappedRangeValueClamped(extentTraceRange, extentTraceCount, xyExtent));
   check(numTracesAroundPuddle > 0);
   int32 numPointsOutside = 0;

   // Shrink the extents a bit - it's fine if just the edges of the puddle are outside
   static constexpr float extentMultiplier = 0.8f;

   // Start the trace up a bit in case we're intersecting with world geometry
   const FVector extraTraceOffset{ 0.0f, 0.0f, 5.0f };

   const float angleOffsetDeg = 360.0f / static_cast<float>(numTracesAroundPuddle);
   FVector prevPosition = FVector::ZeroVector;
   for (int32 i = 0; i < numTracesAroundPuddle; i++)
   {
      const FVector thisPosition = puddle.GetPositionAroundPuddle(i * angleOffsetDeg, extentMultiplier);
#if TAT_ALLOW_PUDDLE_DEBUG
      if (debugDraw && prevPosition != FVector::ZeroVector)
      {
         DrawDebugLine(world, prevPosition, thisPosition, FColor::Cyan, persistentDebugLines, debugDrawLifetime);
      }
#endif
      if (isLocationOutside(thisPosition + extraTraceOffset))
      {
         ++numPointsOutside;
      }
      prevPosition = thisPosition;
   }

   // One extra trace from the center
   ++numTracesAroundPuddle;
   if (isLocationOutside(puddle.Location + extraTraceOffset))
   {
      ++numPointsOutside;
   }

   // If half or more of the traces were outside, then consider the whole puddle outside
   const float outsidePortionNormalized = static_cast<float>(numPointsOutside) / static_cast<float>(numTracesAroundPuddle);
   return outsidePortionNormalized >= 0.5f;
}

// static
bool UTATPuddleUtilities::TargetMatchesPuddleFilter(const AActor* actor, ETATPuddleTargetFilter targetFilter)
{
   if (actor == nullptr)
   {
      return false;
   }

   const ACharacter* character = Cast<ACharacter>(actor);

   // Check if this is a valid target
   switch (targetFilter)
   {
   case ETATPuddleTargetFilter::AllCharacters:
      if (character == nullptr)
      {
         return false;
      }
      break;
   case ETATPuddleTargetFilter::PlayerCharactersOnly:
      if (character == nullptr || !character->IsPlayerControlled())
      {
         return false;
      }
      break;
   case ETATPuddleTargetFilter::NPCsOnly:
      if (character == nullptr || character->IsPlayerControlled())
      {
         return false;
      }
      break;
   default:
      checkNoEntry();
      break;
   }

   return true;
}

// static
FVector UTATPuddleUtilities::GetPositionAroundPuddle(const FVector& location, const FVector& extent, const FRotator& rotation, float angleDeg, float extentMultiplier)
{
   const FQuat orientation = rotation.Quaternion();
   return location
      + (orientation.GetForwardVector() * (extent.X * extentMultiplier * FMath::Cos(FMath::DegreesToRadians(angleDeg))))
      + (orientation.GetRightVector() * (extent.Y * extentMultiplier * FMath::Sin(FMath::DegreesToRadians(angleDeg))))
      + (orientation.GetUpVector() * (extent.Z * extentMultiplier))
   ;
}

// static
ETATPuddleSurfaceAngle UTATPuddleUtilities::GetPuddleSurfaceAngle(const FRotator& rotation)
{
   const float worldUpDot = rotation.Quaternion().GetUpVector().Dot(FVector::UpVector);
   if (worldUpDot > 0.85f)
   {
      return ETATPuddleSurfaceAngle::Floor;
   }
   if (worldUpDot < -0.85f)
   {
      return ETATPuddleSurfaceAngle::Ceiling;
   }
   if (worldUpDot > -0.2f && worldUpDot < 0.2f)
   {
      return ETATPuddleSurfaceAngle::Wall;
   }
   return ETATPuddleSurfaceAngle::Angled;
}

// static
bool UTATPuddleUtilities::PuddleProjectionTrace(const UObject* contextObject, FTATPuddleTransform& puddleTransform, int32 seed, const FVector& impactLocation,
   const FVector& impactDirection, float puddleRadius, const FTATPuddleProjectionTraceParams& projectionTraceParams, const AActor* ignoreActor,
   bool drawDebug, float drawDebugDuration)
{
   puddleTransform.Location = impactLocation;
   puddleTransform.Extent = FVector(puddleRadius, puddleRadius, projectionTraceParams.PuddleThicknessRange.Min);
   puddleTransform.Rotation = FRotator::ZeroRotator;

   UWorld* world = GEngine->GetWorldFromContextObject(contextObject, EGetWorldErrorMode::ReturnNull);
   if (world == nullptr)
   {
      return false;
   }

   if (seed == 0)
   {
      seed = FMath::Rand();
   }
   FRandomStream rng{ seed };

   FCollisionQueryParams queryParams{};
   queryParams.bTraceComplex = projectionTraceParams.TraceComplex;
   queryParams.AddIgnoredActor(ignoreActor);
   FCollisionResponseParams responseParams{};

   auto drawDebugHitNormal = [&](const FHitResult& hit)
   {
      DrawDebugLine(world, hit.Location, hit.Location + (hit.Normal * 50.0f), FColor::Cyan, false, drawDebugDuration, 0, 1.5f);
   };

   FHitResult lineTraceHitResult{};
   auto lineTrace = [&](const FVector& traceStart, const FVector& traceEnd) -> const FHitResult*
   {
      const bool hit = world->LineTraceSingleByChannel(lineTraceHitResult, traceStart, traceEnd, projectionTraceParams.CollisionChannel, queryParams, responseParams);
      if (drawDebug)
      {
         if (hit)
         {
            DrawDebugLine(world, traceStart, lineTraceHitResult.Location, FColor::Green, false, drawDebugDuration);
            drawDebugHitNormal(lineTraceHitResult);
            DrawDebugLine(world, lineTraceHitResult.Location, traceEnd, FColor::Red, false, drawDebugDuration);
         }
         else
         {
            DrawDebugLine(world, traceStart, traceEnd, FColor::White, false, drawDebugDuration);
         }
      }
      return hit ? &lineTraceHitResult : nullptr;
   };

   int32 numHits = 0;
   float hitDistanceMin = FLT_MAX;
   float hitDistanceMax = FLT_MIN;
   float hitDistanceSum = 0.0f;
   PuddleHelpers::FVectorAverage hitNormal{};

   FVector coneOrigin = impactLocation - (impactDirection * puddleRadius * 0.75f);
   const FVector origConeOrigin = coneOrigin;
   const float coneAngleRad = FMath::DegreesToRadians(projectionTraceParams.TraceConeAngleDeg);
   const float medianThicknessTarget = projectionTraceParams.PuddleThicknessRange.Interpolate(0.5f);
   const float adjustAmount = (medianThicknessTarget * 0.75f) / static_cast<float>(projectionTraceParams.TraceCount.Min);

   for (int32 traceIdx = 0; traceIdx < projectionTraceParams.TraceCount.Max; traceIdx++)
   {
      const FVector traceDir = rng.VRandCone(impactDirection, coneAngleRad);
      const FVector traceEnd = coneOrigin + (traceDir * puddleRadius * 1.5f);
      const FHitResult* hitResult = lineTrace(coneOrigin, traceEnd);
      if (hitResult == nullptr)
      {
         continue;
      }

      ++numHits;

      const float dist = FVector::Distance(origConeOrigin, hitResult->Location);
      if (dist < hitDistanceMin)
      {
         hitDistanceMin = dist;
      }
      if (dist > hitDistanceMax)
      {
         hitDistanceMax = dist;
      }
      hitDistanceSum += dist;

      hitNormal.Add(hitResult->Normal);

      // Adjust the cone's origin as we go in the direction of the normals we're finding
      coneOrigin += hitNormal.Get() * adjustAmount;

      // We're aiming to just do the minimum number of traces, but keep going if we have less than two hits
      if (traceIdx >= projectionTraceParams.TraceCount.Min && numHits >= 2)
      {
         break;
      }
   }

   if (numHits == 0)
   {
      return false;
   }

   // Set the thickness to the total distance seen in the traces (clamped to the input min and max values, and halved because it's a box extent)
   const float newThickness = FMath::Clamp(hitDistanceMax - hitDistanceMin, projectionTraceParams.PuddleThicknessRange.Min, projectionTraceParams.PuddleThicknessRange.Max);
   puddleTransform.Extent.Z = newThickness * 0.5f;

   // Recalculate the world location by projecting it out from the cone to the average distance from the line traces.
   float targetDistance = hitDistanceSum / static_cast<float>(numHits);
   // Offset that by half the new thickness value to keep the box centered around the extent
   targetDistance -= newThickness * 0.5f;
   puddleTransform.Location = origConeOrigin + (impactDirection * targetDistance);

   auto getXAxisFromDirection = [](FVector dir, const FVector& zAxis) -> FVector
   {
      dir.Normalize();
      const float dot = dir.Dot(zAxis);
      if (dir.IsNearlyZero() || dot >= 1.0f || dot <= -1.0f)
      {
         // We don't have a useful direction vector for projecting onto the plane defined by the Z axis (eg. it's the same as the Z axis),
         // so just come up with a reasonable arbitrary one. Not ideal but at least we always get a sane result.
         FVector xAxis, yAxis;
         zAxis.FindBestAxisVectors(xAxis, yAxis);
         return xAxis;
      }
      // Project the direction vector onto the plane defined by the average hit normal to produce an X axis
      return dir.ProjectOnToNormal(zAxis);
   };

   // Use the average hit normal as the puddle's Z axis
   const FVector zAxis = hitNormal.GetOr(FVector::UpVector).GetSafeNormal();

   // Generate a rotation based on the Z axis and try to align it based on the impact direction if possible
   puddleTransform.Rotation = FRotationMatrix::MakeFromZX(zAxis, getXAxisFromDirection(impactDirection, zAxis)).Rotator();

   if (drawDebug)
   {
      constexpr int32 numSegments = 16;
      PuddleHelpers::DrawDebugOval(world, impactLocation, FVector2D(puddleTransform.Extent.X, puddleTransform.Extent.Y), puddleTransform.Rotation.Quaternion(),
         numSegments, FColor::Green, false, drawDebugDuration);
   }

   return true;
}

// static
bool UTATPuddleUtilities::RunLiquidParticleExplosionSimulation(
   const UObject* contextObject,
   TArray<FTATLiquidExplosionResult>& particleHits,
   int32 seed,
   const FVector& location,
   const FVector& direction,
   const FFloatInterval& initialLocationOffset,
   const FFloatInterval& initialDirectionalVelocity,
   const FFloatInterval& initialRandomVelocity,
   float initialVelocityRandomConeAngleDeg,
   const FTATLiquidSimulationParams& simulationParams,
   TEnumAsByte<ECollisionChannel> collisionChannel,
   bool drawDebug,
   float drawDebugDuration)
{
   particleHits.Reset();
   if (simulationParams.ParticleCount <= 0 || simulationParams.Runtime <= 0)
   {
      return false;
   }

   UWorld* world = GEngine->GetWorldFromContextObject(contextObject, EGetWorldErrorMode::ReturnNull);
   if (world == nullptr)
   {
      return false;
   }

   if (seed == 0)
   {
      seed = FMath::Rand();
   }

   // Init the particles
   TArray<PuddleHelpers::FLiquidParticle> particles;
   FRandomStream rng{ seed };
   particles.SetNum(simulationParams.ParticleCount);
   for (PuddleHelpers::FLiquidParticle& p : particles)
   {
      FVector initialLocation = location;
      FVector initialVelocity = FVector::ZeroVector;
      const bool haveDirection = !direction.IsNearlyZero();
      if (haveDirection && initialDirectionalVelocity != FFloatInterval(0, 0))
      {
         initialVelocity += direction * rng.FRandRange(initialDirectionalVelocity.Min, initialDirectionalVelocity.Max);
      }
      if (initialRandomVelocity != FFloatInterval(0, 0))
      {
         // If we have a direction and a random cone angle, generate a random direction vector within that cone in that direction.
         const FVector randomDirection = (haveDirection && initialVelocityRandomConeAngleDeg > 0)
            ? rng.VRandCone(direction, FMath::DegreesToRadians(initialVelocityRandomConeAngleDeg))
            : rng.VRand();

         initialVelocity += randomDirection * rng.FRandRange(initialRandomVelocity.Min, initialRandomVelocity.Max);
      }
      if (initialLocationOffset != FFloatInterval(0, 0))
      {
         initialLocation += initialVelocity.GetSafeNormal() * rng.FRandRange(initialLocationOffset.Min, initialLocationOffset.Max);
      }
      p.Init(initialLocation, initialVelocity);
   }

   // Run the simulation
   PuddleHelpers::FLiquidSimulation sim{
      world,
      simulationParams,
      particles,
      FVector(0, 0, world->GetGravityZ()),
      location,
      collisionChannel,
      drawDebug,
      drawDebugDuration,
   };
   sim.RunSimulation();

   // Draw the particle positions at the end of the simulation
   if (drawDebug)
   {
      for (const PuddleHelpers::FLiquidParticle& p : particles)
      {
         if (p.Hit)
         {
            PuddleHelpers::DrawDebugDirectionAndVelocity(world, p.Position, p.Velocity.GetSafeNormal(), p.Velocity.Length(), false, drawDebugDuration, 0, 2.0f);
         }
      }

      DrawDebugString(world, location, FString::Printf(TEXT("Traces: %i"), sim.GetTraceCount()), nullptr, FColor::Cyan, drawDebugDuration, true);
   }

   // Copy relevant particles to the output array
   for (const PuddleHelpers::FLiquidParticle& p : particles)
   {
      if (p.Hit && !p.Dead)
      {
         FTATLiquidExplosionResult& result = particleHits.Emplace_GetRef();
         result.Location = p.Position;
         result.Velocity = p.Velocity;
         result.HitNormal = p.HitNormal;
         result.HitActor = p.HitActor.Get();
      }
   }

   return !particleHits.IsEmpty();
}

// static
bool UTATPuddleUtilities::SimulatePuddleExplosion(
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
   const AActor* ignoreActor,
   bool skipPrimaryPuddle,
   bool drawDebugPuddleProjection,
   bool drawDebugLiquidSimulation,
   float drawDebugDuration)
{
   puddleTransforms.Reset();

   if (simulationParams.Runtime <= 0)
   {
      return false;
   }

   UWorld* world = GEngine->GetWorldFromContextObject(contextObject, EGetWorldErrorMode::ReturnNull);
   if (world == nullptr)
   {
      return false;
   }

   if (seed == 0)
   {
      seed = FMath::Rand();
   }
   FRandomStream rng{ seed };

   int32 projectionTraceCount = 0;

   FTATPuddleTransform basePuddle{};
   if (!PuddleProjectionTrace(contextObject, basePuddle, seed, impactLocation, impactVelocity.GetSafeNormal(), primaryImpactPuddleRadius, projectionTraceParams,
      ignoreActor, drawDebugPuddleProjection, drawDebugDuration))
   {
      return false;
   }
   projectionTraceCount += 1;

   const FQuat basePuddleOrientation = basePuddle.Rotation.Quaternion();
   const FVector basePuddleUpVector = basePuddleOrientation.GetUpVector();
   const ETATPuddleSurfaceAngle basePuddleSurfaceAngle = GetPuddleSurfaceAngle(basePuddle.Rotation);

   // If we're on the ceiling and we have enough particles, treat the last one as a center particle that can fall straight down
   const bool addCenterParticle = simulationParams.ParticleCount > 5 && basePuddleSurfaceAngle == ETATPuddleSurfaceAngle::Ceiling;
   const int32 particlesAroundBasePuddle = addCenterParticle ? (simulationParams.ParticleCount - 1) : simulationParams.ParticleCount;

   // init particles
   float velocityOutwardWeight = 1.0f;
   float velocityUpWeight = 1.0f;
   float velocityImpactWeight = 1.0f;
   float velocityRandomWeight = 1.0f;
   float gravityMultiplier = 1.0f;
   const TCHAR* weightMode = TEXT("");
   if (basePuddleSurfaceAngle == ETATPuddleSurfaceAngle::Floor || basePuddleSurfaceAngle == ETATPuddleSurfaceAngle::Ceiling)
   {
      // On a horizontal-ish surface (floor or ceiling)
      velocityOutwardWeight = 3.0f;
      velocityUpWeight = 3.0f;
      velocityImpactWeight = 0.5f;
      velocityRandomWeight = 0.25f;
      gravityMultiplier = 1.0f;
      weightMode = TEXT("Horiz");
   }
   else if (basePuddleSurfaceAngle == ETATPuddleSurfaceAngle::Wall)
   {
      // On a vertical-ish wall
      velocityOutwardWeight = 1.0f;
      velocityUpWeight = 1.0f;
      velocityImpactWeight = 1.0f;
      velocityRandomWeight = 1.0f;
      gravityMultiplier = 2.0f;
      weightMode = TEXT("Vert");
   }
   else
   {
      // On an angled surface
      ensure(basePuddleSurfaceAngle == ETATPuddleSurfaceAngle::Angled);
      velocityOutwardWeight = 1.0f;
      velocityUpWeight = 0.5f;
      velocityImpactWeight = 0.75f;
      velocityRandomWeight = 0.6f;
      gravityMultiplier = 1.5f;
      weightMode = TEXT("Angled");
   }

   FHitResult particleInitHitResult{};
   FCollisionQueryParams particleInitQueryParams{};
   particleInitQueryParams.bTraceComplex = projectionTraceParams.TraceComplex;
   particleInitQueryParams.AddIgnoredActor(ignoreActor);
   FCollisionResponseParams particleInitResponseParams{};
   int32 particleInitTraceCount = 0;
   auto particleInitLineTrace = [&](const FVector& start, const FVector& end) -> const FHitResult*
   {
      ++particleInitTraceCount;
      const bool hit = world->LineTraceSingleByChannel(particleInitHitResult, start, end, projectionTraceParams.CollisionChannel, particleInitQueryParams, particleInitResponseParams);
      if (drawDebugLiquidSimulation)
      {
         // Offset the endpoints a bit to make it easier to read
         const FVector debugLineStart = particleInitHitResult.TraceStart + ((particleInitHitResult.TraceEnd - particleInitHitResult.TraceStart).GetSafeNormal() * 5.0f);
         const FVector debugLineEnd = particleInitHitResult.TraceEnd + ((particleInitHitResult.TraceStart - particleInitHitResult.TraceEnd).GetSafeNormal() * 5.0f);
         if (hit)
         {
            DrawDebugLine(world, debugLineStart, particleInitHitResult.Location, FColor::Yellow, false, drawDebugDuration);
            DrawDebugPoint(world, particleInitHitResult.Location, 6.0f, FColor::Red, false, drawDebugDuration);
            //DrawDebugLine(world, particleInitHitResult.Location, debugLineEnd, FColor::Orange, false, drawDebugDuration);
         }
         // else
         // {
         //    DrawDebugLine(world, debugLineStart, debugLineEnd, FColor(100, 145, 100), false, drawDebugDuration);
         // }
      }
      return hit ? &particleInitHitResult : nullptr;
   };

   // Init particles
   TArray<PuddleHelpers::FLiquidParticle, TInlineAllocator<16>> particles;
   particles.SetNum(simulationParams.ParticleCount);
   const float angleOffsetRad = (UE_PI * 2.0f) / static_cast<float>(particlesAroundBasePuddle);
   for (int32 i = 0; i < particlesAroundBasePuddle; i++)
   {
      PuddleHelpers::FLiquidParticle& p = particles[i];
      FVector initialPos = PuddleHelpers::PositionOnOval(basePuddle.Location, FVector2D(basePuddle.Extent), basePuddleOrientation, static_cast<float>(i) * angleOffsetRad);

      // Make sure we can get to this particle's start location from the base puddle's origin (otherwise it might be inside some geometry)
      if (const FHitResult* hit = particleInitLineTrace(basePuddle.Location, initialPos))
      {
         // If the particle is too close to the base puddle's origin, just skip it entirely by initializing it as dead
         const float minDistance = FMath::Min(basePuddle.Extent.X, basePuddle.Extent.Y) * 0.5f;
         if (hit->Distance < minDistance)
         {
            p.Init(hit->Location, FVector::ZeroVector);
            p.Dead = true;
            continue;
         }

         // Move the starting location back a bit by 25% of the distance to the hit location
         initialPos = hit->Location - ((initialPos - basePuddle.Location).GetSafeNormal() * (hit->Distance * 0.25f));
      }

      // Figure out the particle's initial velocity based on a number of factors, weighting each one separately
      PuddleHelpers::FVectorAverage initialVelocity{};
      initialVelocity.Add((initialPos - basePuddle.Location).GetSafeNormal(), velocityOutwardWeight);
      initialVelocity.Add(basePuddleUpVector, velocityUpWeight);
      initialVelocity.Add(impactVelocity.GetSafeNormal(), velocityImpactWeight);
      initialVelocity.Add(rng.VRand(), velocityRandomWeight);

      p.Init(initialPos, initialVelocity.Get() * rng.FRandRange(explosionForce.Min, explosionForce.Max));
   }
   // Center particles always start right in the middle of the base puddle and are only used when the base puddle is on a ceiling,
   // so tune it with the assumption that it's going to fall straight down.
   if (addCenterParticle)
   {
      PuddleHelpers::FVectorAverage initialVelocity{};
      initialVelocity.Add(basePuddleUpVector, 1.0f);
      initialVelocity.Add(impactVelocity.GetSafeNormal(), 0.25f);
      initialVelocity.Add(rng.VRand(), 0.25f);
      particles.Last().Init(basePuddle.Location, initialVelocity.Get() * explosionForce.Min);
   }

   // Init the simulation
   PuddleHelpers::FLiquidSimulation sim{
      world,
      simulationParams,
      particles,
      FVector(0.0f, 0.0f, world->GetGravityZ() * gravityMultiplier),
      basePuddle.Location,
      projectionTraceParams.CollisionChannel,
      drawDebugLiquidSimulation,
      drawDebugDuration,
   };
   sim.GetCollisionQueryParamsRef().AddIgnoredActor(ignoreActor);
   sim.GetCollisionQueryParamsRef().bTraceComplex = projectionTraceParams.TraceComplex;

   // Draw the initial positions and velocities
   if (drawDebugLiquidSimulation)
   {
      for (const PuddleHelpers::FLiquidParticle& p : particles)
      {
         DrawDebugPoint(world, p.Position, 5.0f, FColor(200, 200, 200), false, drawDebugDuration);
         PuddleHelpers::DrawDebugDirectionAndVelocity(world, p.Position, p.HitNormal, p.Velocity.Length(), false, drawDebugDuration, 0, 2.0f);
      }
   }

   // Run the simulation
   sim.RunSimulation();

   // Draw the particle positions at their end points
   if (drawDebugLiquidSimulation)
   {
      for (const PuddleHelpers::FLiquidParticle& p : particles)
      {
         FColor color = FColor::White;
         if (p.Hit)
         {
            color = FColor::Green;
         }
         else if (p.Dead)
         {
            color = FColor::Orange;
         }
         DrawDebugPoint(world, p.Position, 7.0f, color, false, drawDebugDuration);

         if (p.Hit)
         {
            PuddleHelpers::DrawDebugDirectionAndVelocity(world, p.Position, p.HitNormal, p.Velocity.Length(), false, drawDebugDuration);
         }
      }
   }

   // For each particle that hit something, either merge it into the base puddle or spawn a new one
   for (const PuddleHelpers::FLiquidParticle& p : particles)
   {
      if (!p.Hit || p.Dead)
      {
         continue;
      }

      if (p.HitNormal.Dot(basePuddleUpVector) >= 0.9f && FVector::DistSquared(p.Position, basePuddle.Location) <= FMath::Square(maxPuddleRadius))
      {
         const FVector positionRelativeToBasePuddle = basePuddleOrientation.UnrotateVector(p.Position);
         const float relVertOffset = FMath::Abs(positionRelativeToBasePuddle.Z - basePuddle.Location.Z);
         if (relVertOffset < projectionTraceParams.PuddleThicknessRange.Max)
         {
            // merge with base puddle
            const float basePuddleRadius = FMath::Max(basePuddle.Extent.X, basePuddle.Extent.Y);
            const float distOffset = FVector::Dist(p.Position, basePuddle.Location) - basePuddleRadius;
            basePuddle.Extent.X = FMath::Max(basePuddle.Extent.X + distOffset, maxPuddleRadius);
            basePuddle.Extent.Y = FMath::Max(basePuddle.Extent.Y + distOffset, maxPuddleRadius);
            const float newExtentZ = FMath::Max(basePuddle.Extent.Z + relVertOffset, projectionTraceParams.PuddleThicknessRange.Max);
            basePuddle.Extent.Z = newExtentZ;
            basePuddle.Location -= basePuddleUpVector * (newExtentZ * 0.5f);
            continue;
         }
      }

      // spawn a new puddle
      FTATPuddleTransform newPuddle{};
      ++projectionTraceCount;
      if (PuddleProjectionTrace(contextObject, newPuddle, seed + projectionTraceCount, p.Position, p.Velocity.GetSafeNormal(), secondaryImpactPuddleRadius,
            projectionTraceParams, ignoreActor, drawDebugPuddleProjection, drawDebugDuration))
      {
         puddleTransforms.Add(newPuddle);
      }
   }

   // Show some stats about the line traces performed
   if (drawDebugPuddleProjection && drawDebugLiquidSimulation)
   {
      const int32 projTraceCount = projectionTraceCount * projectionTraceParams.TraceCount.Min;
      DrawDebugString(world, basePuddle.Location,
         FString::Printf(TEXT("Mode=%s, Traces=Proj(%i) SimSetup(%i) + Sim(%i) = %i"),
            weightMode, projTraceCount, particleInitTraceCount, sim.GetTraceCount(), projTraceCount + particleInitTraceCount + sim.GetTraceCount()),
         nullptr, FColor::Cyan, drawDebugDuration, true);
   }

   if (!skipPrimaryPuddle)
   {
      puddleTransforms.Add(basePuddle);
   }

   return !puddleTransforms.IsEmpty();
}
