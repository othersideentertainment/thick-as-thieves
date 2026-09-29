// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "GameFramework/SafeRoom/TATSafeRoomOverlapVolume.h"

// tat
#include "GameFramework/SafeRoom/TATSafeRoom.h"
#include "GameFramework/SafeRoom/TATSafeRoomClaimAffectedInterface.h"

// ose
#include "Utl/OSEShapeCollisionTrackerComponent.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/OverlapResult.h"
#include "GameplayEffect.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSafeRoomOverlapVolume)

ATATSafeRoomOverlapVolume::ATATSafeRoomOverlapVolume()
{
   PrimaryActorTick.bCanEverTick = false;

   _collisionTrackerComponent = CreateDefaultSubobject<UOSEShapeCollisionTrackerComponent>(TEXT("CollisionShapeTracker"));

   // For now, the only thing this is used for is adding a gameplay effect, which restores ammo,
   // and ensuring that a safe room cannot be claimed if multiple players are inside it
   // Both of these can be done on authority only
   _collisionTrackerComponent->OnlyRunOnAuthority = true;

   // We want to know as soon as a player is inside the safe room, so don't require any delay
   _collisionTrackerComponent->RequiredTimeInsideOfShape = 0.0f;
}


void ATATSafeRoomOverlapVolume::BeginPlay()
{
   Super::BeginPlay();

   if (HasAuthority())
   {
      _collisionTrackerComponent->OnActorEnteredShape.AddUniqueDynamic(this, &ThisClass::_AuthorityOnActorEnterShape);
      _collisionTrackerComponent->OnActorExitedShape.AddUniqueDynamic(this, &ThisClass::_AuthorityOnActorExitShape);

      if (ensure(_safeRoom))
      {
         _AuthorityOnSafeRoomOwningPlayerChanged(_safeRoom->GetOwningPlayer());
         _safeRoom->OnOwningPlayerChanged.AddUniqueDynamic(this, &ThisClass::_AuthorityOnSafeRoomOwningPlayerChanged);
      }
   }
}

void ATATSafeRoomOverlapVolume::_AuthorityOnActorEnterShape(AActor* actor)
{
   check(HasAuthority());

   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(actor))
   {
      if (asc->HasAllMatchingGameplayTags(RequiredPlayerTags))
      {
         for (TSubclassOf<UGameplayEffect> effectToApply : EffectsToApplyToPlayers)
         {
            FGameplayEffectContextHandle effectContext = asc->MakeEffectContext();
            effectContext.AddInstigator(this, this);
            FActiveGameplayEffectHandle effectHandle = asc->ApplyGameplayEffectToSelf(effectToApply.GetDefaultObject(), 0.0f, effectContext);
            _actorsWithEffectApplied.Add(actor, effectHandle);
         }
      }
   }

   BP_AuthorityOnActorEnterVolume(actor);

   if (ensure(_safeRoom))
   {
      _safeRoom->AuthorityActorEnteredSafeRoomVolume(actor);
   }
}

void ATATSafeRoomOverlapVolume::_AuthorityOnActorExitShape(AActor* actor)
{
   check(HasAuthority());

   _actorsWithEffectApplied.CancelByActor(actor);

   BP_AuthorityOnActorExitVolume(actor);

   if (ensure(_safeRoom))
   {
      _safeRoom->AuthorityActorExitedSafeRoomVolume(actor);
   }
}

void ATATSafeRoomOverlapVolume::_AuthorityOnSafeRoomOwningPlayerChanged(ATATPlayerState* owningPlayer)
{
   check(HasAuthority());

   if (owningPlayer && ensure(_safeRoom))
   {
      TInlineComponentArray<UPrimitiveComponent*> primComponents;
      GetComponents(primComponents);

      // Re-use for each component to avoid a bit of churn
      TArray<FOverlapResult> overlapResults;

      // Go through each of our components and try to find any overlapping actors
      TSet<AActor*, DefaultKeyFuncs<AActor*>, TInlineSetAllocator<16>> overlapActors;
      for (UPrimitiveComponent* primComp : primComponents)
      {
         overlapResults.Reset();
         GetWorld()->ComponentOverlapMultiByChannel(overlapResults, primComp, primComp->GetComponentLocation(), primComp->GetComponentRotation(), ECollisionChannel::ECC_WorldDynamic);

         for (const FOverlapResult& result : overlapResults)
         {
            overlapActors.Add(result.GetActor());
         }
      }

      for (AActor* overlappingActor : overlapActors)
      {
         if (auto* affectedByClaim = Cast<ITATSafeRoomClaimAffectedInterface>(overlappingActor))
         {
            affectedByClaim->AuthorityOnSurroundingSafeRoomClaimed(_safeRoom);
         }
      }
   }
}

