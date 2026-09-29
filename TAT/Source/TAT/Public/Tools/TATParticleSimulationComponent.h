// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Environment/TATPuddleUtilities.h"

// ue
#include "Components/SceneComponent.h"

#include "TATParticleSimulationComponent.generated.h"

USTRUCT(BlueprintType)
struct FTATSimulatedParticleState
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FVector PrevPosition = FVector::ZeroVector;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FVector Position = FVector::ZeroVector;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FVector Velocity = FVector::ZeroVector;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FVector HitNormal = FVector::ZeroVector;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   TObjectPtr<AActor> HitActor;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   bool Hit = false;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   bool Dead = false;

   FTATSimulatedParticleState() = default;
   explicit FTATSimulatedParticleState(const PuddleHelpers::FLiquidParticle& particle);
};


/// Runs a CPU particle simulation, allowing the particle data to be used for gameplay (or any other effects)
UCLASS(ClassGroup = (TAT), Blueprintable, BlueprintType, Meta = (BlueprintSpawnableComponent))
class TAT_API UTATParticleSimulationComponent : public USceneComponent
{
   GENERATED_BODY()

public:
   UTATParticleSimulationComponent();
   virtual void OnRegister() override;
   virtual void OnUnregister() override;
   virtual void BeginPlay() override;
   virtual void TickComponent(float deltaTime, enum ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Simulation")
   bool AutoStartSimulation = true;

   /// The initial direction used when the simulation is automatically started
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Simulation", Meta = (EditCondition = "AutoStartSimulation"))
   FVector InitialAutoStartDirection = FVector(1, 0, 0);

   /// How much initial particle velocity to apply in the particle's initial direction when the simulation is automatically started
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Simulation", Meta = (EditCondition = "AutoStartSimulation"))
   FFloatInterval InitialAutoStartDirectionalVelocity = FFloatInterval(250, 500);

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Simulation")
   int32 Seed = 1234;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Simulation")
   FVector Gravity = FVector(0, 0, -980.0f);

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Simulation")
   float GravityMultiplier = 1.0f;

   /// How much initial particle velocity to apply from a random direction vector
   /// Uses InitialVelocityRandomConeAngleDeg to limit the random angle to within a cone of the initial direction
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Simulation")
   FFloatInterval InitialRandomVelocity = FFloatInterval(0, 0);

   /// The angle limit for InitialRandomVelocity
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Simulation", Meta = (UIMin = "0.0", UIMax = "90.0", ForceUnits = "degrees"))
   float InitialVelocityRandomConeAngleDeg = 25.0f;

   /// Push out initial particle locations this far from the origin (in the direction of the particle's initial velocity)
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Simulation")
   FFloatInterval InitialLocationOffset = FFloatInterval(0, 0);

   /// Number of particles to simulate
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Simulation")
   int32 ParticleCount = 10;

   /// If greater than zero, particles that move farther away than this are marked as dead (no longer simulated)
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Simulation", Meta = (UIMin = "0", ClampMin = "0"))
   float MaxRadius = 0.0f;

   /// If greater than zero, particles that move more than this distance under the simulation's origin are marked as dead (no longer simulated)
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Simulation", Meta = (UIMin = "0", ClampMin = "0"))
   float MaxHeight = 0.0f;

   /// What collision channel to use for particle movement traces
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Simulation")
   TEnumAsByte<ECollisionChannel> CollisionChannel = ECC_WorldStatic;

   /// Simulation speed. Setting this to 2.0 will cause the simulation to run at double speed; while a value of 0.5 will run at half speed.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Simulation")
   float TimeMultiplier = 1.0f;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particle Simulation")
   bool DrawDebug = false;

   UFUNCTION(BlueprintCallable)
   void StartSimulation(const FVector& direction, const FFloatInterval& velocityRange, int32 seedOverride = 0);

   UFUNCTION(BlueprintCallable)
   void SetSimulationPaused(bool newPaused);

   UFUNCTION(BlueprintPure)
   FORCEINLINE bool IsSimulationPaused() const { return _simPaused; }

   UFUNCTION(BlueprintPure)
   bool IsSimulationRunning() const;

   UFUNCTION(BlueprintCallable)
   void StopSimulation();

   UFUNCTION(BlueprintPure)
   FORCEINLINE int32 GetParticleCount() const { return _particles.Num(); }

   UFUNCTION(BlueprintPure)
   FORCEINLINE int32 GetActiveParticleCount() const { return _numActiveParticles; }

   UFUNCTION(BlueprintPure)
   bool GetParticleState(int32 particleIndex, FTATSimulatedParticleState& particleState) const;

   FORCEINLINE const PuddleHelpers::FLiquidParticle& GetParticleStateChecked(int32 particleIndex) const
   {
      check(_particles.IsValidIndex(particleIndex));
      return _particles[particleIndex];
   }

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSimLifecycleEvent);

   UPROPERTY(BlueprintAssignable)
   FSimLifecycleEvent OnSimulationStart;

   UPROPERTY(BlueprintAssignable)
   FSimLifecycleEvent OnSimulationStop;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSimUpdate, int32, numAliveParticles);
   UPROPERTY(BlueprintAssignable)
   FSimUpdate OnSimulationUpdate;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FParticleHit, int32, particleIndex, const FHitResult&, hit);
   UPROPERTY(BlueprintAssignable)
   FParticleHit OnParticleHit;

private:
   void _InitParticles(const FVector& initialDirection, const FFloatInterval& initialVelocityRange, int32 seed);

   bool _simPaused = false;
   TUniquePtr<PuddleHelpers::FLiquidSimulation> _sim;
   TArray<PuddleHelpers::FLiquidParticle> _particles;
   int32 _numActiveParticles = 0;

};
