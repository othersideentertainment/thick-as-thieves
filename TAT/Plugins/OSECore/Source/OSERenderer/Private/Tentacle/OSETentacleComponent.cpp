// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Tentacle/OSETentacleComponent.h"

// ue5
#include "DrawDebugHelpers.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSETentacleComponent)

DEFINE_LOG_CATEGORY_STATIC(LogOSETentacle, Log, All);

DECLARE_STATS_GROUP(TEXT("Tentacles"), STATGROUP_Tentacles, STATCAT_Advanced);

//---------------------------------------------------------------------------------------
// Console vars
//---------------------------------------------------------------------------------------

namespace Tentacle
{
   namespace CVar
   {
      namespace Debug
      {
         TAutoConsoleVariable<int32> Enabled (
            TEXT("OSE.Tentacle.Debug"),
            0,
            TEXT("Enables or disables the tentacle debug visualization"),
            ECVF_Default);
      }

      namespace Visuals
      {
         TAutoConsoleVariable<int32> LocalSpace(
            TEXT("OSE.Tentacle.Visuals.LocalSpace"),
            0,
            TEXT("When enabled passes local space coordinates to the FX system"),
            ECVF_Default);
      }

      TAutoConsoleVariable<int32> UpdateRetractPosition(
         TEXT("OSE.Tentacle.UpdateRetractPosition"),
         0,
         TEXT("When enabled will update the world space target while retracting. When disabled will use the last attached position"),
         ECVF_Default);
   }
}


//---------------------------------------------------------------------------------------
// UOSETentacleComponent
//---------------------------------------------------------------------------------------

UOSETentacleComponent::UOSETentacleComponent()
{
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bAllowTickOnDedicatedServer = false;

   // @TODO: May not want to use object types; probably switch to actor's movement channels
   _collisionTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
   _collisionTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
}

void UOSETentacleComponent::BeginPlay()
{
   Super::BeginPlay();

   // @TODO: Configure additional settings; use channels
   _objectQueryParams = FCollisionObjectQueryParams(_collisionTypes);
   _collisionQueryParams = FCollisionQueryParams(SCENE_QUERY_STAT(UOSETentacleComponent), true, GetOwner());

   _localOrigin = FVector::Zero();
   _worldOrigin = GetComponentTransform().TransformPosition(_localOrigin);
   _prevWorldOrigin = _worldOrigin;

   _timeSinceLastExtend = 1;

   _extendArray.Reserve(_tentacleCount);
   _retractArray.Reserve(_tentacleCount);
   _visualDataArray.SetNum(_tentacleCount);
   _visualDataRemap.SetNumUninitialized(_tentacleCount);
   _nextAvailFX = 0;

   for (int32 i = 0; i < _visualDataRemap.Num(); ++i)
   {
      _visualDataRemap[i] = i;
   }

   if (auto* niagaraTemplate = Cast<UNiagaraSystem>(_visualFXTemplate))
   {
      // Keeping FX components in this component space means we can design the FX system
      // such that it can assume it's system is oriented like this component.
      // If that changes, the parameters we set here, and the FX itself, will need to change.
      _visualFXComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
         niagaraTemplate,                             // UNiagaraSystem * SystemTemplate,
         this,                                        // USceneComponent * AttachToComponent,
         NAME_None,                                   // FName AttachPointName,
         FVector::ZeroVector,                         // FVector Location,
         FRotator::ZeroRotator,                       // FRotator Rotation,
         EAttachLocation::Type::KeepRelativeOffset,   // EAttachLocation::Type LocationType,
         true);                                       // bool bAutoDestroy,
   }
   else
   {
      // No template was specified; try to find a direct child
      const TArray<TObjectPtr<USceneComponent>>& attachedComponents = GetAttachChildren();
      for (USceneComponent* component : attachedComponents)
      {
         if (auto visualComponent = Cast<UNiagaraComponent>(component))
         {
            _visualFXComponent = visualComponent;
            break;
         }
      }
   }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

   if (_visualFXComponent == nullptr)
   {
      UE_LOG(LogOSETentacle, Warning, TEXT("No visual FX component specified or found as a child of component [%s]"), *GetNameSafe(this));
   }
#endif
}

// Called every frame
void UOSETentacleComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   const FOSETentacleSettings settings = GetSettings();

   // Scope our main tick function so we can accurately reflect peformance overhead without including debug visualizations
   {
      DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Tick"), STAT_Tentacle_Tick, STATGROUP_Tentacles);

      // Update our origin first; these are used throughout the tick function
      const FTransform componentToWorld = GetComponentTransform();
      _localOrigin = FVector::Zero();
      _prevWorldOrigin = _worldOrigin;
      _worldOrigin = componentToWorld.TransformPosition(_localOrigin);

      // Local distance offset is how much we moved since last tick; it's used to offset our distance checks
      // so that we bias towards tentacles in the direction we're moving
      const FVector worldVelocity = (deltaTime > 0) ? (_worldOrigin - _prevWorldOrigin) / deltaTime : FVector::Zero();
      const FVector worldVelocityClamped = worldVelocity.GetClampedToMaxSize(FMath::Min(settings.Speed.ExtendSpeed, settings.Speed.RetractSpeed));
      const FVector localDistanceOffset = componentToWorld.InverseTransformVectorNoScale(worldVelocityClamped * deltaTime);

      // Our sphere distribution can change at runtime since we support dynamic radii changes
      _UpdateSphereDistribution();

      // Keep track of in-use indices so we don't spawn another tentacle there
      TSet<uint16> usedSphereIndices;

      // The longest time remaining for any extending tentacles
      float longestTimeRemaining = 0;

      // Update extending tentacles
      {
         DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Tick.Extend"), STAT_Tentacle_Tick_Extend, STATGROUP_Tentacles);

         for (auto itr = _extendArray.CreateIterator(); itr; ++itr)
         {
            const FOSETentacleState prevState = (*itr);
            check(prevState.HitResult.IsValidBlockingHit());
            check(prevState.SphereIdx > INDEX_NONE);

            usedSphereIndices.Add(prevState.SphereIdx);

            const FVector worldHitLocation = prevState.GetWorldSpaceLocation();
            const FVector localHitLocation = componentToWorld.InverseTransformPosition(worldHitLocation);
            const FVector localDirection = (localHitLocation - _localOrigin).GetSafeNormal();

            // All of our rotations are properly oriented, rotator-style pitch and yaw (with no roll),
            // so this should maintain our local space up vector the same way the distributed points do.
            const FQuat sphereRotation = settings.Spawn.AdjustSphereLocation ? localDirection.ToOrientationQuat() : prevState.SphereRotation;
            const FFloatInterval length = _GetLengthForRotation(sphereRotation);

            // @TODO: !!! Check the current length BEFORE performing the trace.
            // It runs the risk of ending the extension prematurely, as the trace 
            // determines the _actual_ distance. But in that case it should be a pretty
            // significant performance boost if we can skip doing traces.

            FHitResult newHit;
            if (
               (prevState.TimeRemaining <= 0) ||
               (!_IsValidSphereRotation(sphereRotation, length)) ||
               (!_PerformTrace(newHit, prevState.HitResult, sphereRotation, worldHitLocation, length))
               )
            {
               // Set our time remaining to "zero" for retracting tentacles, so we can easily
               // distinguish between extending vs retracting.
               // Set it to `deltaTime` so that the retract update will zero it out.
               FOSETentacleState retractState = prevState;
               retractState.TimeRemaining = deltaTime;
               _retractArray.Add(retractState);

               OnTentacleBeginRetract.Broadcast(retractState.HitResult);

               itr.RemoveCurrent();
               continue;
            }

            const float targetLength = newHit.Distance;

            FOSETentacleState nextState = prevState;
            nextState.HitResult = newHit;
            nextState.UpdateLength(targetLength, settings.Speed.ExtendSpeed, deltaTime);
            nextState.UpdateComponentSpaceLocation();

            // Distance from target
            const float distanceFromTarget = FMath::Abs(targetLength - nextState.Length);

            // Update the life time if we begin aging immediately or have reached our target
            if (settings.Spawn.BeginAgingImmediately || (distanceFromTarget <= settings.Size.TentacleRadius))
            {
               nextState.TimeRemaining -= deltaTime;
            }

            // Update longest time remaining
            longestTimeRemaining = FMath::Max(longestTimeRemaining, nextState.TimeRemaining);

            *itr = nextState;
         }
      }

      // Update retracting tentacles
      {
         DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Tick.Retract"), STAT_Tentacle_Tick_Retract, STATGROUP_Tentacles);

         for (auto itr = _retractArray.CreateIterator(); itr; ++itr)
         {
            const FOSETentacleState prevState = (*itr);
            check(prevState.HitResult.IsValidBlockingHit());
            check(prevState.SphereIdx > INDEX_NONE);

            usedSphereIndices.Add(prevState.SphereIdx);

            // Using the hit result location is the last known location, while the `GetWorldSpaceLocation` version
            // will update it to the current position in world space.
            const bool updateRetractPosition = (Tentacle::CVar::UpdateRetractPosition.GetValueOnAnyThread() != 0);
            const FVector worldHitLocation = updateRetractPosition ? prevState.GetWorldSpaceLocation() : FVector(prevState.HitResult.Location);

            const FVector localHitLocation = componentToWorld.InverseTransformPosition(worldHitLocation);
            const FVector localDirection = settings.Spawn.AdjustSphereLocation ? (localHitLocation - _localOrigin).GetSafeNormal() : prevState.SphereRotation.Vector();

            const FVector localSurface = _localOrigin + (localDirection * settings.Size.BodyRadius);
            const FVector worldSurface = componentToWorld.TransformPosition(localSurface);

            // We set the trace start location for retracting tentacles, even without performing a trace.
            // This is in order to have consistent results with extended tentacles which perform a trace.
            FOSETentacleState nextState = prevState;
            nextState.TimeRemaining -= deltaTime;
            nextState.HitResult.TraceStart = worldSurface;
            nextState.HitResult.Location = worldHitLocation;
            nextState.HitResult.ImpactPoint = worldHitLocation;
            nextState.UpdateLength(0, settings.Speed.RetractSpeed, deltaTime);

            if (nextState.Length <= 0)
            {
               _ReleaseVisualDataIndex(nextState.VisualDataIdx);
               itr.RemoveCurrent();
               continue;
            }

            *itr = nextState;
         }
      }

      // Extend new tentacles
      {
         DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Tick.TryNew"), STAT_Tentacle_Tick_TryNew, STATGROUP_Tentacles);

         const int32 totalCount = _extendArray.Num() + _retractArray.Num();
         check(totalCount <= _tentacleCount);

         // Check the timer to see if we should extend them yet
         const int32 extendRate = settings.Spawn.NewTentacleExtendRate;
         const float extendInterval = (extendRate <= 0) ? 0.0f : (1.0f / float(extendRate));
         _timeSinceLastExtend += deltaTime;

         const int32 numToAdd = _tentacleCount - totalCount;
         if ((_timeSinceLastExtend >= extendInterval) && (numToAdd > 0))
         {
            int32 numAdded = 0;
            for (int32 i = 0; i < numToAdd; ++i)
            {
               FQuat sphereRotation;
               FFloatInterval length;
               if (_GetNextSphereRotation(sphereRotation, length))
               {
                  /*
                  // @TODO: Disabled for now.
                  // @TODO: Verify we only want offsets that decrease the max length
                  const float distanceOffset = FMath::Min(0.0f, sphereRotation.GetForwardVector() | localDistanceOffset);
                  maxLength = FMath::Max(0.0f, maxLength + distanceOffset);
                  */

                  // Apply the new query offset
                  const float halfQueryLength = settings.Length.QueryLengthDecrease * 0.5f;
                  length.Min += halfQueryLength;
                  length.Max -= halfQueryLength;
                  length.Max = FMath::Max(length.Min, length.Max);
                  if (length.Size() > UE_KINDA_SMALL_NUMBER)
                  {
                     const int32 uniqueSphereIdx = _currSphereIndex % _numSpherePoints;
                     if (!usedSphereIndices.Contains(uniqueSphereIdx))
                     {
                        FOSETentacleState nextState;
                        if (_PerformTrace(nextState.HitResult, FHitResult(), sphereRotation, length))
                        {
                           _timeSinceLastExtend = 0;
                           ++numAdded;

                           // Add this to the list in case we try to add more than one
                           usedSphereIndices.Add(uniqueSphereIdx);

                           // Update lifetime min / max if we're making sure to persist as long as the tentacle with the longest life span
                           const float lifeTimeMin = settings.Spawn.WaitForLongestTimeRemaining ? FMath::Max(settings.Spawn.LifeTime.Min, longestTimeRemaining) : settings.Spawn.LifeTime.Min;
                           const float lifeTimeMax = FMath::Max(settings.Spawn.LifeTime.Max, lifeTimeMin);

                           nextState.SphereIdx = uniqueSphereIdx;
                           nextState.SphereRotation = sphereRotation;
                           nextState.Length = 0;
                           nextState.TimeRemaining = FMath::FRandRange(lifeTimeMin, lifeTimeMax);
                           nextState.VisualDataIdx = _AcquireVisualDataIndex();
                           nextState.UpdateComponentSpaceLocation();

                           OnTentacleBeginExtend.Broadcast(nextState.HitResult);

                           _extendArray.Add(nextState);
                           break;
                        }
                     }
                  }
               }
            }

            // Offset our next sphere index by a fraction of the ones we still have to add.
            // This helps stagger the search pattern somewhat, so it's not scanning and
            // attaching new tentacles all in the same direction
            check(numAdded <= numToAdd);
            const int32 numRemaining = (numToAdd - numAdded);
            _currSphereIndex += _numSpherePoints / (numRemaining + 1);
         }
      }

      // Update all visual data
      _UpdateVisualData();
   }

#if ENABLE_DRAW_DEBUG

   if (Tentacle::CVar::Debug::Enabled.GetValueOnAnyThread() != 0)
   {
      _DrawDebugState();
   }

#endif // ENABLE_DRAW_DEBUG
}

void UOSETentacleComponent::_UpdateSphereDistribution()
{
   const FOSETentacleSettings settings = GetSettings();

   // Use the rectangular region that contains a tentacle as the per-tentacle area
   const float tentacleArea = FMath::Square(2 * settings.Size.TentacleRadius);

   // The area of our sphere
   const float sphereArea = 4 * UE_PI * FMath::Square(settings.Size.BodyRadius);

   // Make sure we have at least twice the number of tentacles
   const int32 minSpherePoints = FMath::Max(10, _tentacleCount * 2);
   _numSpherePoints = (uint16)FMath::Max(minSpherePoints, int32(sphereArea / tentacleArea));
}

float UOSETentacleComponent::_GetMinLength() const
{
   // Tentacle diameter. We use the length properties for new tentacles only.
   return 2 * GetSettings().Size.TentacleRadius;
}

FFloatInterval UOSETentacleComponent::_GetLengthForRotation(const FQuat& inRotation) const
{
   const FOSETentacleSettings settings = GetSettings();

   // Default length property
   FFloatInterval result = settings.Length.Length;

   if (settings.Length.EnableSeparateAxisLengths)
   {
      const FVector rotationDirection = inRotation.GetForwardVector();

      const FVector lengthPerAxisMin(
         (rotationDirection.X >= 0.0f ? settings.Length.LengthFront.Min : settings.Length.LengthBack.Min  ),
         (rotationDirection.Y >= 0.0f ? settings.Length.LengthRight.Min : settings.Length.LengthLeft.Min  ),
         (rotationDirection.Z >= 0.0f ? settings.Length.LengthTop.Min   : settings.Length.LengthBottom.Min));

      const FVector lengthPerAxisMax(
         (rotationDirection.X >= 0.0f ? settings.Length.LengthFront.Max : settings.Length.LengthBack.Max  ),
         (rotationDirection.Y >= 0.0f ? settings.Length.LengthRight.Max : settings.Length.LengthLeft.Max  ),
         (rotationDirection.Z >= 0.0f ? settings.Length.LengthTop.Max   : settings.Length.LengthBottom.Max));

      // A unit normal squared sums to one
      const FVector normalSquared = rotationDirection * rotationDirection;
      result.Min = normalSquared | lengthPerAxisMin;
      result.Max = normalSquared | lengthPerAxisMax;
   }

   result.Min = FMath::Max(result.Min, _GetMinLength());
   result.Max = FMath::Max(result.Min, result.Max);
   check(result.IsValid());

   return result;
}

bool UOSETentacleComponent::_IsValidSphereRotation(const FQuat& inRotation, const FFloatInterval& inLength) const
{
   // Check if the length is valid
   if (inLength.Size() <= UE_KINDA_SMALL_NUMBER)
   {
      return false;
   }

   // Check if it's within the forward cone we exclude
   const float excludeAngle = GetSettings().Spawn.ExcludeConeAngle;
   if (excludeAngle > UE_KINDA_SMALL_NUMBER)
   {
      const FVector rotationDirection = inRotation.GetForwardVector();
      const float forwardAngleDeg = FMath::RadiansToDegrees(FMath::Acos(rotationDirection.X));
      if ((forwardAngleDeg * 2) < excludeAngle)
      {
         // Falls inside the angle limit
         return false;
      }
   }

   // Valid
   return true;
}

bool UOSETentacleComponent::_GetSphereRotation(int32 inSphereIdx, FQuat& outRotation, FFloatInterval& outLength) const
{
   // Used to evenly distribute points on the sphere per an offset Fibonacci lattice:
   // http://extremelearning.com.au/how-to-evenly-distribute-points-on-a-sphere-more-effectively-than-the-canonical-fibonacci-lattice/
   //
   const float epsilon = 0.36f;
   const float n = _numSpherePoints;
   const float i = (inSphereIdx % _numSpherePoints);

   const float theta = TWO_PI * i / UE_GOLDEN_RATIO;
   const float phi = FMath::Acos(1 - 2 * (i + epsilon) / (n - 1 + 2 * epsilon));

   // Convert standard polar theta / phi to rotator yaw / pitch, oriented at the forward vector
   const FRotator rotator = FRotator(
      FMath::RadiansToDegrees(phi) - 90, FMath::RadiansToDegrees(theta), 0).GetNormalized();

   // Always set the output values; we can inspect it if we need to know why it failed
   outRotation = FQuat::MakeFromRotator(rotator);
   outLength = _GetLengthForRotation(outRotation);

   return _IsValidSphereRotation(outRotation, outLength);
}

bool UOSETentacleComponent::_GetNextSphereRotation(FQuat& outRotation, FFloatInterval& outLength)
{
   // Advances the index prior to calling the method
   return _GetSphereRotation(++_currSphereIndex, outRotation, outLength);
}

bool UOSETentacleComponent::_PerformTrace(FHitResult& outHitResult, const FHitResult& inPreviousHit, const FQuat& inSphereRotation, const FFloatInterval& inLength) const
{
   const FOSETentacleSettings settings = GetSettings();

   const FVector localSurface = _localOrigin + (inSphereRotation.GetForwardVector() * settings.Size.BodyRadius);
   const FVector worldSurface = GetComponentTransform().TransformPosition(localSurface);
   const FVector worldDirection = (worldSurface - _worldOrigin).GetSafeNormal();

   return _PerformTrace(outHitResult, inPreviousHit, inSphereRotation, worldSurface + worldDirection, inLength);
}

// @TODO: Switch to async traces
bool UOSETentacleComponent::_PerformTrace(FHitResult& outHitResult, const FHitResult& inPreviousHit, const FQuat& inSphereRotation, const FVector& inWorldTargetPos, const FFloatInterval& inLength) const
{
   check(inLength.IsValid());

   DECLARE_SCOPE_CYCLE_COUNTER(TEXT("PerformTrace"), STAT_Tentacle_PerformTrace, STATGROUP_Tentacles);

   const FOSETentacleSettings settings = GetSettings();

   const FVector localSurface = _localOrigin + (inSphereRotation.GetForwardVector() * settings.Size.BodyRadius);
   const FVector worldSurface = GetComponentTransform().TransformPosition(localSurface);
   const FVector worldDirection = (worldSurface - _worldOrigin).GetSafeNormal();

   const FVector worldTargetDirection = (inWorldTargetPos - worldSurface).GetSafeNormal();
   const FVector worldTargetPosition = worldSurface + (worldTargetDirection * inLength.Max);

   // Make sure our targeting direction extends outwards from the sphere surface
   if ((worldDirection | worldTargetDirection) > 0)
   {
      if (GetWorld()->LineTraceSingleByObjectType(outHitResult, worldSurface, worldTargetPosition, _objectQueryParams, _collisionQueryParams))
      {
         if (outHitResult.IsValidBlockingHit())
         {
            if (outHitResult.Distance >= inLength.Min)
            {
               // Checks if we hit a different object
               if (!inPreviousHit.IsValidBlockingHit() || (inPreviousHit.GetHitObjectHandle() == outHitResult.GetHitObjectHandle()))
               {
                  return true;
               }
            }
         }
      }
   }

   return false;
}

int32 UOSETentacleComponent::_AcquireVisualDataIndex()
{
   check(_nextAvailFX >= 0);
   check(_nextAvailFX < _visualDataRemap.Num());

   const int32 currVisualIdx = _visualDataRemap[_nextAvailFX];
   _nextAvailFX++;

   check(currVisualIdx >= 0);
   check(currVisualIdx < _visualDataArray.Num());
   return currVisualIdx;
}

void UOSETentacleComponent::_ReleaseVisualDataIndex(int32 inVisualIdx)
{
   check(_nextAvailFX > 0);
   check(_nextAvailFX <= _visualDataRemap.Num());
   _nextAvailFX--;

   check(inVisualIdx >= 0);
   check(inVisualIdx < _visualDataArray.Num());

   const int32 numRemoved = _visualDataRemap.RemoveSingle(inVisualIdx);
   check(numRemoved == 1);

   _visualDataRemap.Add(inVisualIdx);
}

FTransform UOSETentacleComponent::_GetVisualLocalToWorldTransform() const
{
   // Whether or not to use local space coordinates for the visuals
   const bool useLocalSpace = (Tentacle::CVar::Visuals::LocalSpace.GetValueOnGameThread() != 0);
   if (useLocalSpace)
   {
      if (auto* niagaraComponent = Cast<UNiagaraComponent>(_visualFXComponent.Get()))
      {
         return niagaraComponent->GetComponentTransform();
      }
      else
      {
         return GetComponentTransform();
      }
   }
   
   return FTransform::Identity;
}

void UOSETentacleComponent::_UpdateVisualData(FOSETentacleVisualData& visualData, const FOSETentacleState& state, const FTransform& localToWorld) const
{
   const FOSETentacleSettings settings = GetSettings();

   const float defaultBodyRadius = settings.Size.BodyRadius;
   const float defaultArmRadius  = settings.Size.TentacleRadius;
   const float startOffsetBuffer = FMath::Min(defaultBodyRadius * 0.5f, defaultArmRadius);

   // Get positions in world space initially; we can then transform these and derive all other data
   const FVector worldSpaceOrigin  = _worldOrigin;
   const FVector worldSpaceEnd     = state.HitResult.Location;
   const FVector worldSpaceDelta   = worldSpaceEnd - state.HitResult.TraceStart;
   const FVector worldSpaceUnitDir = worldSpaceDelta.GetSafeNormal();
   const FVector worldSpaceStart   = (worldSpaceEnd - worldSpaceDelta) - (worldSpaceUnitDir * startOffsetBuffer);
   const FVector worldSpaceCurrent = worldSpaceStart + (worldSpaceUnitDir * (state.Length + startOffsetBuffer));

   // Transform these to visual local space (it may be world space if the transform is identity)
   const FVector localSpaceOrigin  = localToWorld.InverseTransformPosition(worldSpaceOrigin);
   const FVector localSpaceStart   = localToWorld.InverseTransformPosition(worldSpaceStart);
   const FVector localSpaceEnd     = localToWorld.InverseTransformPosition(worldSpaceEnd);
   const FVector localSpaceCurrent = localToWorld.InverseTransformPosition(worldSpaceCurrent);

   // Update the length values
   const float lengthTotal      = (localSpaceEnd - localSpaceStart).Size();
   const float lengthCurrent    = (localSpaceCurrent - localSpaceStart).Size();
   const float lengthNormalized = (lengthTotal > 0) ? lengthCurrent / lengthTotal : 0;

   // Update radii values
   const float bodyRadius     = (localSpaceStart - localSpaceOrigin).Size();
   const float tentacleRadius = defaultArmRadius;

   // Update time and state values
   const int32 sphereIdx     = state.SphereIdx;
   const float timeRemaining = state.TimeRemaining;
   const bool isExtending    = timeRemaining > 0;
   const bool isRetracting   = !isExtending;

   // Set the visual data
   {
      visualData.BodyOrigin         = localSpaceOrigin;
      visualData.BodyRadius         = bodyRadius;
      visualData.RootPosition       = localSpaceStart;
      visualData.RootRadius         = tentacleRadius;
      visualData.TipPositionTarget  = localSpaceEnd;
      visualData.TipPositionCurrent = localSpaceCurrent;
      visualData.LengthTotal        = lengthTotal;
      visualData.LengthNormalized   = lengthNormalized;
      visualData.LengthCurrent      = lengthCurrent;
      visualData.ExtendWeight       = isExtending ? 1 : 0;
      visualData.RetractWeight      = isRetracting ? 1 : 0;
      visualData.TimeRemaining      = timeRemaining;
      visualData.SphereIdx          = sphereIdx;
   }
}

void UOSETentacleComponent::_UpdateVisualData(const FOSETentacleState& state, const FTransform& localToWorld)
{
   // Set the visual data
   const int32 currVisualIdx = state.VisualDataIdx;
   check(currVisualIdx >= 0);
   check(currVisualIdx < _visualDataArray.Num());

   // The visual data we're updating
   FOSETentacleVisualData& visualData = _visualDataArray[currVisualIdx];
   _UpdateVisualData(visualData, state, localToWorld);
}

void UOSETentacleComponent::_UpdateVisualData()
{
   DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UpdateVisualData"), STAT_Tentacle_UpdateVisualData, STATGROUP_Tentacles);

   const FOSETentacleSettings settings = GetSettings();
   const FTransform visualLocalToWorld = _GetVisualLocalToWorldTransform();

   // Update visual data that is active and in use
   {
      DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UpdateVisualData.Active"), STAT_Tentacle_UpdateVisualData_Active, STATGROUP_Tentacles);

      for (auto itr = _extendArray.CreateConstIterator(); itr; ++itr)
      {
         _UpdateVisualData(*itr, visualLocalToWorld);
      }

      for (auto itr = _retractArray.CreateConstIterator(); itr; ++itr)
      {
         _UpdateVisualData(*itr, visualLocalToWorld);
      }
   }

   // Update unused, inactive visual data as well. We try to provide data that will still play nice
   // with an FX system that is not necessarily aware if the visuals are active or not.
   {
      DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UpdateVisualData.Inactive"), STAT_Tentacle_UpdateVisualData_Inactive, STATGROUP_Tentacles);

      for (int32 idx = _nextAvailFX; idx < _visualDataRemap.Num(); ++idx)
      {
         // Get the existing visual data
         FOSETentacleVisualData visualData = GetVisualData(idx);
         const int32 sphereIdx = visualData.SphereIdx;

         // We don't care if this check fails; we just want the sphere rotation
         FQuat sphereRotation;
         FFloatInterval length;
         _GetSphereRotation(sphereIdx, sphereRotation, length);

         // Use the surface values from rotation
         const FVector localSurface = _localOrigin + (sphereRotation.GetForwardVector() * FMath::Max(1, settings.Size.BodyRadius - settings.Size.TentacleRadius));
         const FVector worldSurface = GetComponentTransform().TransformPosition(localSurface);

         // Keep the current point just inside of the end point. Every other
         // piece of data can be derived from this.
         const FVector worldSpaceOrigin  = _worldOrigin;
         const FVector worldSpaceStart   = worldSpaceOrigin;
         const FVector worldSpaceEnd     = worldSurface;
         const FVector worldSpaceCurrent = worldSpaceEnd + ((worldSpaceStart - worldSpaceEnd).GetSafeNormal() * settings.Size.TentacleRadius);

         // Transform these to visual local space
         const FVector localSpaceOrigin  = visualLocalToWorld.InverseTransformPosition(worldSpaceOrigin);
         const FVector localSpaceStart   = visualLocalToWorld.InverseTransformPosition(worldSpaceStart);
         const FVector localSpaceEnd     = visualLocalToWorld.InverseTransformPosition(worldSpaceEnd);
         const FVector localSpaceCurrent = visualLocalToWorld.InverseTransformPosition(worldSpaceCurrent);

         // Update the length values
         const float lengthTotal      = (localSpaceEnd - localSpaceStart).Size();
         const float lengthCurrent    = (localSpaceCurrent - localSpaceStart).Size();
         const float lengthNormalized = (lengthTotal > 0) ? lengthCurrent / lengthTotal : 0;

         // Update radii values
         const float bodyRadius     = (localSpaceStart - localSpaceOrigin).Size();
         const float tentacleRadius = settings.Size.TentacleRadius;

         // Neither extending nor retracting is a hint that this is inactive
         const float timeRemaining = 0;
         const bool isExtending    = false;
         const bool isRetracting   = false;

         // Set the updated visual data
         visualData.BodyOrigin         = localSpaceOrigin;
         visualData.BodyRadius         = bodyRadius;
         visualData.RootPosition       = localSpaceStart;
         visualData.RootRadius         = tentacleRadius;
         visualData.TipPositionTarget  = localSpaceEnd;
         visualData.TipPositionCurrent = localSpaceCurrent;
         visualData.LengthTotal        = lengthTotal;
         visualData.LengthNormalized   = lengthNormalized;
         visualData.LengthCurrent      = lengthCurrent;
         visualData.ExtendWeight       = isExtending ? 1 : 0;
         visualData.RetractWeight      = isRetracting ? 1 : 0;
         visualData.TimeRemaining      = timeRemaining;
         visualData.SphereIdx          = sphereIdx;

         _SetVisualData(idx, visualData);
      }
   }
}

const FOSETentacleVisualData& UOSETentacleComponent::GetVisualData(int32 index) const
{
   // Remap to the direct index
   return _GetVisualData_DirectIdx(_visualDataRemap[index]);
}

void UOSETentacleComponent::GetVisualDataArray(TArray<FOSETentacleVisualData>& outDataArray) const
{
   const int32 visualCount = _visualDataArray.Num();
   outDataArray.SetNumUninitialized(visualCount);

   for (int32 idx = 0; idx < visualCount; ++idx)
   {
      // Not remapped; we want to provide consistent results to consumers of this data
      outDataArray[idx] = _GetVisualData_DirectIdx(idx);
   }
}

void UOSETentacleComponent::_SetVisualData(int32 index, const FOSETentacleVisualData& visualData)
{
   // Remap to the direct index
   _SetVisualData_DirectIdx(_visualDataRemap[index], visualData);
}

#if ENABLE_DRAW_DEBUG

void UOSETentacleComponent::_DrawDebugState() const
{
   TArray<FOSETentacleVisualData> dataArray;
   GetVisualDataArray(dataArray);

   for (auto itr = dataArray.CreateConstIterator(); itr; ++itr)
   {
      _DrawDebugState(*itr);
   }
}

void UOSETentacleComponent::_DrawDebugState(const FOSETentacleVisualData& visualData) const
{
   const uint8 debugColorHue = (uint8)(255.0f * (visualData.SphereIdx % _numSpherePoints) / (_numSpherePoints - 1));
   const FColor debugColor1 = FLinearColor::MakeFromHSV8(debugColorHue, 255, 255).ToFColor(true);
   const FColor debugColor2 = FColor::Silver;
   const FVector debugStart = visualData.RootPosition;
   const FVector debugEnd = visualData.TipPositionTarget;
   const FVector debugCurrent = visualData.TipPositionCurrent;
   const float debugRadius = visualData.RootRadius;

   DrawDebugLine(GetWorld(), debugCurrent, debugEnd, debugColor2);
   DrawDebugSphere(GetWorld(), debugEnd, debugRadius * 0.5f, 10, debugColor2);

   DrawDebugLine(GetWorld(), debugStart, debugCurrent, debugColor1);
   DrawDebugSphere(GetWorld(), debugStart, debugRadius, 20, debugColor1);
   DrawDebugSphere(GetWorld(), debugCurrent, debugRadius, 20, debugColor1);
}

#endif // ENABLE_DRAW_DEBUG
