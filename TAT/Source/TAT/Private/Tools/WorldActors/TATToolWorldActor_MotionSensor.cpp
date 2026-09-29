// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/WorldActors/TATToolWorldActor_MotionSensor.h"

// ose
#include "Utl/OSEShapeCollisionTrackerComponent.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATToolWorldActor_MotionSensor)

ATATToolWorldActor_MotionSensor::ATATToolWorldActor_MotionSensor()
{
   NetDormancy = DORM_DormantAll;
   bAlwaysRelevant = true;

   _collisionTrackerComponent = CreateDefaultSubobject<UOSEShapeCollisionTrackerComponent>(TEXT("CollisionShapeTracker"));
   // We want this to run on all so the clients can update their glyph colours
   _collisionTrackerComponent->OnlyRunOnAuthority = false;
}

void ATATToolWorldActor_MotionSensor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   // Since this is only used for displaying toasts, it only needs to be replicated to the owner
   DOREPLIFETIME_CONDITION(ATATToolWorldActor_MotionSensor, _numActorEntrancesToArea, COND_OwnerOnly);
}

void ATATToolWorldActor_MotionSensor::BeginPlay()
{
   Super::BeginPlay();

   _collisionTrackerComponent->OnActorEnteredShape.AddUniqueDynamic(this, &ThisClass::_OnActorEnteredShape);
   _collisionTrackerComponent->OnActorExitedShape.AddUniqueDynamic(this, &ThisClass::_OnActorExitedShape);
}

bool ATATToolWorldActor_MotionSensor::GetClosestActorDistance(float& distance) const
{
   const FVector ourLocation = GetActorLocation();

   float closestDistanceSqr = FLT_MAX;
   bool hasClosestActor = false;
   for (const TWeakObjectPtr<AActor>& actorPtr : _validActorsInArea)
   {
      if (const AActor* actor = actorPtr.Get())
      {
         const float distSqrToActor = FVector::DistSquared(actor->GetActorLocation(), ourLocation);
         closestDistanceSqr = FMath::Min(closestDistanceSqr, distSqrToActor);
         hasClosestActor = true;
      }
   }

   // If no valid actor, return 0 as something more reasonable than FLT_MAX
   distance = hasClosestActor ? FMath::Sqrt(closestDistanceSqr) : 0.0f;
   return hasClosestActor;
}

bool ATATToolWorldActor_MotionSensor::_ShouldTrackActor(const AActor* actor) const
{
   // Ignore our instigator (the player who placed us)
   if (actor == GetInstigator())
   {
      return false;
   }

   // Check tag requirements
   if (const UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(actor))
   {
      if (asc->HasAllMatchingGameplayTags(RequiredTargetTags))
      {
         if (!asc->HasAnyMatchingGameplayTags(BlockedTargetTags))
         {
            return true;
         }
      }
   }

   return false;
}

void ATATToolWorldActor_MotionSensor::_OnActorEnteredShape(AActor* actor)
{
   if (_ShouldTrackActor(actor))
   {
      const bool isFirstDetectedActor = (_validActorsInArea.Num() == 0);

      ensure(!_validActorsInArea.Contains(actor));
      _validActorsInArea.AddUnique(actor);

      // If we now have actors we're tracking, call into BP and start ticking
      if (isFirstDetectedActor)
      {
         SetActorTickEnabled(true);
         OnActorsStartDetected();
      }

      // On authority, mark when a relevant actor enters the area
      // If this is a listen server, fire the toast locally
      // Increment the counter used to replicate to the remote owner how many times this has happened, so they can display the toasts
      if (HasAuthority())
      {
         FlushNetDormancy();
         _numActorEntrancesToArea++;

         if (GetInstigator()->IsLocallyControlled())
         {
            OnRelevantActorDetectedOnLocalClient();
         }
      }
   }
}

void ATATToolWorldActor_MotionSensor::_OnActorExitedShape(AActor* actor)
{
   const int32 startingNumActors = _validActorsInArea.Num();

   _validActorsInArea.Remove(actor);

   // Clean up any stale actors
   _validActorsInArea.RemoveAll([](const TWeakObjectPtr<AActor>& actor)
   {
      return !actor.IsValid();
   });

   // If we no longer have actors we're tracking, call into BP and stop ticking
   if (_validActorsInArea.Num() == 0 && startingNumActors > 0)
   {
      SetActorTickEnabled(false);
      OnActorsNoLongerDetected();
   }
}

void ATATToolWorldActor_MotionSensor::_OnRep_NumActorEntrancesToArea(int32 oldNumActorEntrancesToArea)
{
   // Possession changes on escape mean that our instigator may no longer exist
   if (const APawn* instigator = GetInstigator())
   {
      // This replication should only occur to the owning pawn, but there may be some instances,
      // e.g. possession changes on death, that it would be false
      if (instigator->IsLocallyControlled())
      {
         for (int32 i = oldNumActorEntrancesToArea; i < _numActorEntrancesToArea; i++)
         {
            OnRelevantActorDetectedOnLocalClient();
         }
      }
   }
}
