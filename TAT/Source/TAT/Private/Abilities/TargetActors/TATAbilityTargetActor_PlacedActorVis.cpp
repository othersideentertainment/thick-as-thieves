// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/TargetActors/TATAbilityTargetActor_PlacedActorVis.h"

// ue5
#include "Abilities/GameplayAbility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAbilityTargetActor_PlacedActorVis)

DEFINE_LOG_CATEGORY_STATIC(LogTATAbilityTargetActor_PlacedActorVis, Log, All)

ATATAbilityTargetActor_PlacedActorVis::ATATAbilityTargetActor_PlacedActorVis()
{
   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = true;
}

void ATATAbilityTargetActor_PlacedActorVis::Tick(float deltaSeconds)
{
   // N.B. Deliberately not calling Super::Tick, since we are overriding the behavior in AGameplayAbilityTargetActor_Trace::Tick
   // to get access to the hit result. Still calling AActor::Tick to ensure we get BP tick events, or run any latent actions tied to us
   AActor::Tick(deltaSeconds);

   if (SourceActor && SourceActor->GetLocalRole() != ENetRole::ROLE_SimulatedProxy)
   {
      FHitResult hitResult = PerformTrace(SourceActor);
      bool isValidTarget = _AllowHitAsTarget(hitResult);
      UpdateTargetingVisuals(hitResult, isValidTarget);
   }
}

bool ATATAbilityTargetActor_PlacedActorVis::_AllowHitAsTarget(const FHitResult& hit)
{
   if (AActor* playerActor = OwningAbility->GetAvatarActorFromActorInfo())
   {
      return UTATToolWorldActorConstraint::CanPlaceWorldActor(WorldActorConstraint, playerActor, hit);
   }
   else
   {
      UE_LOG(LogTATAbilityTargetActor_PlacedActorVis, Warning, TEXT("Could not find avatar actor for ability '%s'"), *OwningAbility.GetName());
      return false;
   }
}

