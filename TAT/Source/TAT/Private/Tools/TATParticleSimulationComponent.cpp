// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/TATParticleSimulationComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATParticleSimulationComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATParticleSimulationComponent, Log, All)

FTATSimulatedParticleState::FTATSimulatedParticleState(const PuddleHelpers::FLiquidParticle& particle)
   : PrevPosition(particle.PrevPosition)
   , Position(particle.Position)
   , Velocity(particle.Velocity)
   , HitNormal(particle.HitNormal)
   , HitActor(particle.HitActor.Get())
   , Hit(particle.Hit)
   , Dead(particle.Dead)
{
}

UTATParticleSimulationComponent::UTATParticleSimulationComponent()
{
   PrimaryComponentTick.bCanEverTick = true;
}

void UTATParticleSimulationComponent::OnRegister()
{
   Super::OnRegister();
}

void UTATParticleSimulationComponent::OnUnregister()
{
   Super::OnUnregister();
}

void UTATParticleSimulationComponent::BeginPlay()
{
   Super::BeginPlay();

   if (AutoStartSimulation)
   {
      StartSimulation(GetComponentQuat().RotateVector(InitialAutoStartDirection.GetSafeNormal()), InitialAutoStartDirectionalVelocity);
   }
}

void UTATParticleSimulationComponent::TickComponent(float deltaTime, enum ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   _numActiveParticles = 0;

   if (_sim && !_simPaused && !_sim->IsFinished())
   {
      _numActiveParticles = _sim->Update(deltaTime * TimeMultiplier);

      OnSimulationUpdate.Broadcast(_numActiveParticles);

      if (_numActiveParticles == 0)
      {
         StopSimulation();
      }
   }

   if (!_sim)
   {
      return;
   }

#if UE_ENABLE_DEBUG_DRAWING
   if (DrawDebug)
   {
      constexpr bool persistentLines = false;
      const float drawDuration = PrimaryComponentTick.TickInterval * 1.03f;

      UWorld* world = GetWorld();
      for (const PuddleHelpers::FLiquidParticle& p : _particles)
      {
         FColor color = FColor(0, 255, 255);
         if (p.Dead) { color = FColor(243, 156, 18); }
         else if (p.Hit) { color = FColor(90, 190, 190); }

         const float squaredSpeed = p.Velocity.SquaredLength();
         if (p.IsAlive() && squaredSpeed >= FMath::Square(1.0f))
         {
            const float relSpeed = FMath::GetMappedRangeValueClamped(FVector2D(2.0, 200.0), FVector2D(0.0, 1.0), FMath::Sqrt(squaredSpeed));
            const FVector start = p.Position - (p.Velocity.GetSafeNormal() * FMath::Lerp(0.05f, 10.0f, relSpeed));
            DrawDebugDirectionalArrow(world, start, p.Position, FMath::Lerp(3.0f, 10.0f, relSpeed), color, persistentLines, drawDuration);
            DrawDebugPoint(world, p.Position, 4.0f, FColor::White, persistentLines, drawDuration);
         }
         else
         {
            DrawDebugPoint(world, p.Position, 7.0f, color, persistentLines, drawDuration);
         }
      }
   }
#endif // UE_ENABLE_DEBUG_DRAWING
}

void UTATParticleSimulationComponent::StartSimulation(const FVector& direction, const FFloatInterval& velocityRange, int32 seedOverride)
{
   if (_sim)
   {
      StopSimulation();
   }

   const int32 seed = (seedOverride != 0) ? seedOverride : Seed;

   _InitParticles(direction, velocityRange, seed);
   if (_particles.IsEmpty())
   {
      return;
   }

   FTATLiquidSimulationParams params{};
   params.ParticleCount = _particles.Num();
   params.MaxRadius = MaxRadius;
   params.MaxHeight = MaxHeight;
   params.GravityMultiplier = GravityMultiplier;

   _sim = MakeUnique<PuddleHelpers::FLiquidSimulation>(
      GetWorld(),
      params,
      _particles,
      Gravity,
      GetComponentLocation(),
      CollisionChannel,
      DrawDebug,
      0.0f);

   if (AActor* owner = GetOwner())
   {
      _sim->GetCollisionQueryParamsRef().AddIgnoredActor(owner);
   }

   _sim->SetCollisionCallback([weakThis = MakeWeakObjectPtr(this)](int32 particleIdx, const FHitResult& hitResult)
   {
      if (UTATParticleSimulationComponent* self = weakThis.Get())
      {
         self->OnParticleHit.Broadcast(particleIdx, hitResult);
      }
   });

   OnSimulationStart.Broadcast();
}

void UTATParticleSimulationComponent::SetSimulationPaused(bool newPaused)
{
   _simPaused = newPaused;
}

bool UTATParticleSimulationComponent::IsSimulationRunning() const
{
   return _sim != nullptr && !_sim->IsFinished();
}

void UTATParticleSimulationComponent::StopSimulation()
{
   if (_sim == nullptr)
   {
      return;
   }

   OnSimulationStop.Broadcast();

   _sim.Reset();
}

bool UTATParticleSimulationComponent::GetParticleState(int32 particleIndex, FTATSimulatedParticleState& particleState) const
{
   if (!_particles.IsValidIndex(particleIndex))
   {
      particleState = {};
      return false;
   }
   particleState = FTATSimulatedParticleState(_particles[particleIndex]);
   return true;
}

void UTATParticleSimulationComponent::_InitParticles(const FVector& initialDirection, const FFloatInterval& initialVelocityRange, int32 seed)
{
   if (seed == 0)
   {
      seed = FMath::Rand();
   }

   FRandomStream rng{ seed };
   _particles.SetNumZeroed(FMath::Max(0, ParticleCount));

   if (_particles.IsEmpty())
   {
      return;
   }

   for (PuddleHelpers::FLiquidParticle& p : _particles)
   {
      FVector initialLocation = GetComponentLocation();
      FVector initialVelocity = FVector::ZeroVector;
      const bool haveDirection = !initialDirection.IsNearlyZero();
      if (haveDirection && initialVelocityRange != FFloatInterval(0, 0))
      {
         initialVelocity += initialDirection * rng.FRandRange(initialVelocityRange.Min, initialVelocityRange.Max);
      }
      if (InitialRandomVelocity != FFloatInterval(0, 0))
      {
         // If we have a direction and a random cone angle, generate a random direction vector within that cone in that direction.
         const FVector randomDirection = (haveDirection && InitialVelocityRandomConeAngleDeg > 0)
            ? rng.VRandCone(initialDirection, FMath::DegreesToRadians(InitialVelocityRandomConeAngleDeg))
            : rng.VRand();

         initialVelocity += randomDirection * rng.FRandRange(InitialRandomVelocity.Min, InitialRandomVelocity.Max);
      }
      if (InitialLocationOffset != FFloatInterval(0, 0))
      {
         initialLocation += initialVelocity.GetSafeNormal(1e-06, FVector::UpVector) * rng.FRandRange(InitialLocationOffset.Min, InitialLocationOffset.Max);
      }
      p.Init(initialLocation, initialVelocity);
   }
}

