// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameFramework/Actor.h"

// tat
#include "Indicators/TATClientProxyActorInterface.h"

#include "TATTransientParticleActor.generated.h"

class UNiagaraComponent;

/// Actor class used for spawning Niagara particle system indicators at runtime.
/// Primarily intended as a native base class for thief vision indicator glyphs.
UCLASS(Blueprintable)
class TAT_API ATATTransientParticleActor : public AActor, public ITATClientProxyActorInterface
{
   GENERATED_BODY()

   ATATTransientParticleActor();

public:
   // From ITATClientProxyActorInterface
   virtual void OnSpawnedAsClientProxy_Implementation(float remainingLifeSpan, float indicatorLifeSpan, const FTATClientProxySpawnParams& spawnParams) override;
   virtual void OnClientProxyActorLifeSpanRefreshed_Implementation(float newRemainingLifeSpan) override;
   virtual void OnClientProxyActorSetVisible_Implementation(bool newVisible) override;
   virtual void OnDestroyClientProxy_Implementation() override;

   /// Called when particles should be made visible
   UFUNCTION(BlueprintNativeEvent, Category = "Transient Particle Actor")
   void OnActivateParticles(float remainingLifeSpan, float totalLifeSpan);
   virtual void OnActivateParticles_Implementation(float remainingLifeSpan, float totalLifeSpan);

   /// Called when particles should be made invisible
   UFUNCTION(BlueprintNativeEvent, Category = "Transient Particle Actor")
   void OnDeactivateParticles();
   virtual void OnDeactivateParticles_Implementation();

   UNiagaraComponent* GetParticleComponent() const { return _particleComponent; }

   /// Gets the amount of time this indicator has left.
   UFUNCTION(BlueprintPure, Category = "Transient Particle Actor")
   float GetIndicatorLifeSpanRemaining() const;

protected:
   double _worldTimeAtSpawnOrRefresh = 0.0;
   float _remainingLifeSpanAtSpawn = 0.0f;
   float _indicatorLifeSpan = 0.0f;

   /// How long to keep this actor alive after the indicator should go away.
   /// This is just intended to give particle systems enough time to fade out before being destroyed.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Transient Particle Actor")
   float _lifeSpanAfterDestroyRequest = 1.0f;

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Transient Particle Actor")
   USceneComponent* _particleRootComponent = nullptr;

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Transient Particle Actor")
   UNiagaraComponent* _particleComponent = nullptr;
};
