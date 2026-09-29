// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Indicators/TATTransientParticleActor.h"

// tat
#include "NiagaraCommon.h"
#include "NiagaraShared.h"
#include "NiagaraDataInterface.h"

// ue
#include "NiagaraComponent.h"
#include "GameFramework/GameStateBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTransientParticleActor)
DEFINE_LOG_CATEGORY_STATIC(LogTATTransientParticleActor, Log, All);

ATATTransientParticleActor::ATATTransientParticleActor()
{
   _particleRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("ParticleRootComponent"));
   _particleComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ParticleComponent"));
   _particleComponent->bAutoActivate = false;
   _particleComponent->SetupAttachment(_particleRootComponent);
   SetRootComponent(_particleRootComponent);
}

void ATATTransientParticleActor::OnSpawnedAsClientProxy_Implementation(float remainingLifeSpan, float indicatorLifeSpan, const FTATClientProxySpawnParams& spawnParams)
{
   _worldTimeAtSpawnOrRefresh = GetWorld()->GetTimeSeconds();
   _remainingLifeSpanAtSpawn = remainingLifeSpan;
   _indicatorLifeSpan = indicatorLifeSpan;
   OnActivateParticles(remainingLifeSpan, indicatorLifeSpan);
}

void ATATTransientParticleActor::OnClientProxyActorLifeSpanRefreshed_Implementation(float newRemainingLifeSpan)
{
   _worldTimeAtSpawnOrRefresh = GetWorld()->GetTimeSeconds();
   _remainingLifeSpanAtSpawn = newRemainingLifeSpan;
}

void ATATTransientParticleActor::OnClientProxyActorSetVisible_Implementation(bool newVisible)
{
   if (newVisible)
   {
      OnActivateParticles(GetIndicatorLifeSpanRemaining(), _indicatorLifeSpan);
   }
   else
   {
      OnDeactivateParticles();
   }
}

void ATATTransientParticleActor::OnDestroyClientProxy_Implementation()
{
   OnDeactivateParticles();

   if (_lifeSpanAfterDestroyRequest > 0)
   {
      SetLifeSpan(_lifeSpanAfterDestroyRequest);
   }
   else
   {
      Destroy();
   }
}

void ATATTransientParticleActor::OnActivateParticles_Implementation(float remainingLifeSpan, float totalLifeSpan)
{
   if (_particleComponent != nullptr)
   {
      _particleComponent->Activate();
   }
}

void ATATTransientParticleActor::OnDeactivateParticles_Implementation()
{
   if (_particleComponent != nullptr)
   {
      _particleComponent->Deactivate();
   }
}

float ATATTransientParticleActor::GetIndicatorLifeSpanRemaining() const
{
   if (_worldTimeAtSpawnOrRefresh > 0)
   {
      const float timeSinceSpawn = static_cast<float>(GetWorld()->GetTimeSeconds() - _worldTimeAtSpawnOrRefresh);
      return FMath::Clamp(_remainingLifeSpanAtSpawn - timeSinceSpawn, 0.0f, _indicatorLifeSpan);
   }
   return 0.0f;
}
