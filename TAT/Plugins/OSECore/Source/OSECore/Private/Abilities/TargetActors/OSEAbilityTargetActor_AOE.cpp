// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/TargetActors/OSEAbilityTargetActor_AOE.h"
#include "GameFramework/Pawn.h"
#include "WorldCollision.h"
#include "Abilities/GameplayAbility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAbilityTargetActor_AOE)

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//   AOSEAbilityTargetActor_AOE
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

AOSEAbilityTargetActor_AOE::AOSEAbilityTargetActor_AOE(const FObjectInitializer& ObjectInitializer)
   : Super(ObjectInitializer)
{
   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

void AOSEAbilityTargetActor_AOE::StartTargeting(UGameplayAbility* InAbility)
{
   Super::StartTargeting(InAbility);
   _DestroyWorldReticles();
   SourceActor = InAbility->GetCurrentActorInfo()->AvatarActor.Get();
}

bool AOSEAbilityTargetActor_AOE::ShouldProduceTargetData() const
{
   // Workaround for ShouldProduceTargetData being false on the server for AI, because it is erroneously only checking
   // MasterPC, which is explicitly a PlayerController. This is preferable to setting ShouldProduceTargetDataOnServer,
   // as that will have other, potentially undesirable side-effects on the server.
   // TODO: Consider engine mod?
   // NOTE: If you change this, also update the other ShouldProduceTargetData methods with the same fix.
   return Super::ShouldProduceTargetData() || (OwningAbility && OwningAbility->GetActorInfo().IsLocallyControlled());
}

void AOSEAbilityTargetActor_AOE::ConfirmTargetingAndContinue()
{
   check(ShouldProduceTargetData());
   if (SourceActor)
   {
      TArray<TWeakObjectPtr<AActor>> actors;
      PerformOverlap(actors);
      FGameplayAbilityTargetDataHandle Handle = MakeTargetData(actors);
      TargetDataReadyDelegate.Broadcast(Handle);
   }
}

FGameplayAbilityTargetDataHandle AOSEAbilityTargetActor_AOE::MakeTargetData(const TArray<TWeakObjectPtr<AActor>>& actors) const
{
   if (OwningAbility)
   {
      /** Use the source location instead of the literal origin */
      return StartLocation.MakeTargetDataHandleFromActors(actors, false);
   }

   return FGameplayAbilityTargetDataHandle();
}

void AOSEAbilityTargetActor_AOE::_DestroyWorldReticles()
{
   for (TWeakObjectPtr<AGameplayAbilityWorldReticle>& reticle : _reticleActors)
   {
      if (reticle.IsValid())
      {
         reticle->Destroy();
      }
   }
   _reticleActors.Empty();
}

void AOSEAbilityTargetActor_AOE::PerformOverlap(TArray<TWeakObjectPtr<AActor>>& result, bool positionForPreview)
{
}

void AOSEAbilityTargetActor_AOE::UpdateReticles(const TArray<TWeakObjectPtr<AActor>>& actors)
{
   if (ReticleClass == nullptr) return;

   // ensure enough reticles are created
   while (actors.Num() > _reticleActors.Num())
   {
      AGameplayAbilityWorldReticle* spawnedReticleActor = GetWorld()->SpawnActor<AGameplayAbilityWorldReticle>(ReticleClass, GetActorLocation(), GetActorRotation());
      if (!spawnedReticleActor) break;

      spawnedReticleActor->InitializeReticle(this, PrimaryPC, ReticleParams);
      _reticleActors.Add(spawnedReticleActor);
   }

   // update current reticles
   // NOTE: currently assumes that reticle actors don't need to stay with the same actor. If there are reticles with particles or something that you could tell if they swap, we may need to do something fancier later. But for now, will keep it simple
   const int32 count = FMath::Min(actors.Num(), _reticleActors.Num());
   for (int i = 0; i < count; ++i)
   {
      AActor* targetActor = actors[i].Get();
      check(targetActor);

      if (AGameplayAbilityWorldReticle* reticleActor = _reticleActors[i].Get())
      {
         reticleActor->SetActorHiddenInGame(false);
         reticleActor->SetActorLocation(targetActor->GetActorLocation());
         reticleActor->SetIsTargetAnActor(true);
      }
   }

   // disable irrelevant reticles
   for (int i = count; i < _reticleActors.Num(); ++i)
   {
      if (AGameplayAbilityWorldReticle* reticleActor = _reticleActors[i].Get())
      {
         reticleActor->SetActorHiddenInGame(true);
         reticleActor->SetIsTargetAnActor(false);
      }
   }
}

void AOSEAbilityTargetActor_AOE::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
   Super::EndPlay(EndPlayReason);
   _DestroyWorldReticles();
}

void AOSEAbilityTargetActor_AOE::Tick(float DeltaSeconds)
{
   if (SourceActor)
   {
      TArray<TWeakObjectPtr<AActor>> actors;
      PerformOverlap(actors, true);
      UpdateReticles(actors);
   }
}

