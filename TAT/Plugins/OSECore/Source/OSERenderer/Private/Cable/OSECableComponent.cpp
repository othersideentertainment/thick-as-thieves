// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Cable/OSECableComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSECableComponent)

// Begins play for this component. Occurs at level startup or actor spawn
// This is before BeginPlay (Actor or Component).
// All Components(that want initialization) in the level will be Initialized on load before any Actor / Component gets BeginPlay.
void UOSECableComponent::BeginPlay()
{
   // Not ticked
   HasBeenTicked = false;
   TotalLength = 0.0f;

   Super::BeginPlay();
}

// Ends play for this component. Called from AActor::EndPlay only if bHasBegunPlay is true
void UOSECableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
   // Not ticked
   HasBeenTicked = false;
   TotalLength = 0.0f;

   Super::EndPlay(EndPlayReason);
}

// Called every frame
void UOSECableComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
   Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

   // Ticked now
   HasBeenTicked = true;
   TotalLength = 0.0f;

   // Copy and cache the particle locations
   GetCableParticleLocations(CachedParticles);

   // Allocate cached distances array
   const int32 NumParticles = CachedParticles.Num();
   CachedDistances.Empty(NumParticles);
   CachedDistances.Add(0.0f);

   // Calculate total length
   if (NumParticles >= 2)
   {
      for (int32 i = 1; i < NumParticles; ++i)
      {
         const FVector& PosPrev = CachedParticles[i - 1];
         const FVector& PosNext = CachedParticles[i - 0];
         TotalLength += FVector::Distance(PosPrev, PosNext);
         CachedDistances.Add(TotalLength);
      }
   }
}

// Returns the world-space particle transform at the specified length in world space. This is based on the total cable length.
// If the length is less than zero or greater than the total length, this function will return false.
// The transform will always be set to a valid value, clamped to the length.
bool UOSECableComponent::GetCableTransformAtLength(FTransform& Transform, float Length) const
{
   const int32 NumParticles = CachedParticles.Num();
   if (NumParticles < 2)
   {
      // Not enough particles to form a cable segment.
      // Just return the first particle transform.
      Transform = GetCableTransformAtIndex(0);
      return false;
   }

   if (Length <= 0.0f)
   {
      // Zero-length or less.
      // Just return the first particle transform.
      Transform = GetCableTransformAtIndex(0);
      return false;
   }
   else if (Length >= TotalLength)
   {
      // Max length or greater.
      // Just return the last particle transform.
      Transform = GetCableTransformAtIndex(NumParticles-1);
      return false;
   }
   
   // Iterate through all segments
   for (int32 i = 1; i < NumParticles; ++i)
   {
      const float LenPrev = CachedDistances[i - 1];
      const float LenNext = CachedDistances[i - 0];
      
      // Check if this distance will take us to our length
      if (LenNext >= Length)
      {
         const float RangePct = FMath::GetRangePct(LenPrev, LenNext, Length);
         const float ClampedPct = FMath::Clamp(RangePct, 0.0f, 1.0f);
         
         const FTransform XfmCurr = GetCableTransformAtIndex(i - 1);
         const FTransform XfmNext = GetCableTransformAtIndex(i - 0);
         Transform.Blend(XfmCurr, XfmNext, ClampedPct);
         return true;
      }
   }

   // Not found; return end point
   // @TODO: Should this ever happen?
   Transform = GetCableTransformAtIndex(NumParticles - 1);
   return false;
}

// Same as GetCableTransformAtLength, except the length is specified from the end point.
bool UOSECableComponent::GetCableTransformAtReverseLength(FTransform& Transform, float ReverseLength) const
{
   // This method handles negative / out of range values correctly, so we can simply subtract length
   return GetCableTransformAtLength(Transform, TotalLength - ReverseLength);
}

// Returns the world-space particle transform at the specified index. If the index is out of range, it will clamp it.
FTransform UOSECableComponent::GetCableTransformAtIndex(int32 ParticleIndex) const
{
   const FTransform LocalToWorldXfm = GetComponentTransform();

   // Return component transform if we don't have enough points to make a single segment
   const int32 NumParticles = CachedParticles.Num();
   if (NumParticles < 2)
      return LocalToWorldXfm;

   int32 IndexCurr;
   int32 IndexPrev;
   int32 IndexNext;

   // Find suitable previous and next particle positions. We get smoother orientations
   // along the cable by not using the particle's position directly (unless we have to).
   if (ParticleIndex <= 0)
   {
      IndexCurr = 0;
      IndexPrev = (IndexCurr - 0);
      IndexNext = (IndexCurr + 1);
   }
   else if (ParticleIndex >= (NumParticles - 1))
   {
      IndexCurr = (NumParticles - 1);
      IndexPrev = (IndexCurr - 1);
      IndexNext = (IndexCurr + 0);
   }
   else
   {
      IndexCurr = (ParticleIndex);
      IndexPrev = (IndexCurr - 1);
      IndexNext = (IndexCurr + 1);
   }

   const FVector Position = CachedParticles[IndexCurr];
   const FVector Direction = CachedParticles[IndexNext] - CachedParticles[IndexPrev];

   const FMatrix RotationMatrix = FRotationMatrix::MakeFromXZ(Direction, LocalToWorldXfm.GetUnitAxis(EAxis::Z));
   return FTransform(RotationMatrix.ToQuat(), Position, FVector::OneVector);
}

void UOSECableComponent::SampleCableTransformsUniformly(float offset, float length, int samples, TArray<FTransform>& transforms)
{
   if (transforms.Num() != samples)
   {
      transforms.SetNum(samples);
   }

   float t = offset;
   int particleIdx = 0;
   for (int idx = 0; idx < samples; ++idx)
   {
      while (particleIdx + 1 < CachedDistances.Num() && (CachedDistances[particleIdx] > t || CachedDistances[particleIdx + 1] <= t))
      {
         particleIdx++;
      }

      int nextParticleIdx = particleIdx + 1 < CachedDistances.Num() ? particleIdx + 1 : particleIdx;

      FTransform current = GetCableTransformAtIndex(particleIdx);
      FTransform next = GetCableTransformAtIndex(nextParticleIdx);

      const float rangePct = FMath::GetRangePct(CachedDistances[particleIdx], CachedDistances[nextParticleIdx], t);
      const float clampedPct = FMath::Clamp(rangePct, 0.0f, 1.0f);

      transforms[idx].Blend(current, next, clampedPct);
      
      t += length / samples;
   }
}

void UOSECableComponent::SampleCableTransformsUniformlyReversed(float offset, float length, int samples, TArray<FTransform>& transforms)
{
   if (transforms.Num() != samples)
   {
      transforms.SetNum(samples);
   }

   float t = TotalLength - offset;
   int particleIdx = CachedParticles.Num() - 1;
   for (int idx = 0; idx < samples; ++idx)
   {
      while (particleIdx - 1 >= 0 && (CachedDistances[particleIdx] < t || CachedDistances[particleIdx - 1] >= t))
      {
         particleIdx--;
      }

      int nextParticleIdx = particleIdx - 1 >= 0 ? particleIdx - 1 : particleIdx;

      FTransform current = GetCableTransformAtIndex(particleIdx);
      FTransform next = GetCableTransformAtIndex(nextParticleIdx);

      const float rangePct = FMath::GetRangePct(CachedDistances[particleIdx], CachedDistances[nextParticleIdx], t);
      const float clampedPct = FMath::Clamp(rangePct, 0.0f, 1.0f);

      transforms[idx].Blend(current, next, clampedPct);

      t -= length / samples;
   }
}

