// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/Tasks/AbilityTask_TrackActorsInSphere.h"

// ue4
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_TrackActorsInSphere)

UAbilityTask_TrackActorsInSphere* UAbilityTask_TrackActorsInSphere::TrackActorsInSphere(UGameplayAbility* owningAbility, float sphereRadius, float extraLeaveDistance, float pollInterval, FCollisionProfileName collisionProfile, FGameplayTargetDataFilterHandle targetFilter, bool drawDebug /*= false*/)
{
   UAbilityTask_TrackActorsInSphere* task = NewAbilityTask<UAbilityTask_TrackActorsInSphere>(owningAbility);
   task->_sphereRadius = sphereRadius;
   task->_extraRemoveDistance = extraLeaveDistance;
   task->_pollInterval = pollInterval;
   task->_collisionProfile = collisionProfile;
   task->_targetFilter = targetFilter;
   task->_drawDebug = drawDebug;
   return task;
}

void UAbilityTask_TrackActorsInSphere::Activate()
{
   Super::Activate();

   _Update();
   if (UWorld* world = GetWorld())
   {
      world->GetTimerManager().SetTimer(_timerHandle, this, &UAbilityTask_TrackActorsInSphere::_Update, _pollInterval, true);
   }
}

void UAbilityTask_TrackActorsInSphere::OnDestroy(bool abilityIsEnding)
{
   if (UWorld* world = GetWorld())
   {
      world->GetTimerManager().ClearTimer(_timerHandle);
   }

   _trackedActors.Empty();

   Super::OnDestroy(abilityIsEnding);
}

void UAbilityTask_TrackActorsInSphere::_Update()
{
   AActor* avatar = AbilitySystemComponent->AbilityActorInfo->AvatarActor.Get();
   if (avatar)
   {
      _RemoveActorsOutOfRange(avatar);
      _AddActorsInRange(avatar);
   }
}

void UAbilityTask_TrackActorsInSphere::_RemoveActorsOutOfRange(AActor* avatar)
{
   const float removeDistanceSqr = FMath::Square(_sphereRadius + _extraRemoveDistance);
   _trackedActors.RemoveAllSwap([avatar, removeDistanceSqr, this](AActor* candidate)
      {
         if (!IsValid(candidate))
         {
            // still fire it in case it is helpful (separate callback?)
            _OnActorRemoved(candidate);
            return true;
         }

         if (avatar->GetSquaredDistanceTo(candidate) > removeDistanceSqr)
         {
            _OnActorRemoved(candidate);
            return true;
         }

         return false;
      });
}

void UAbilityTask_TrackActorsInSphere::_AddActorsInRange(AActor* avatar)
{
   FCollisionQueryParams params(SCENE_QUERY_STAT(TrackActorsInSphere), false);
   params.AddIgnoredActor(avatar);
   TArray<FOverlapResult> overlaps;
   FCollisionObjectQueryParams objectParams(ECC_Pawn);

   UWorld* world = GetWorld();
   check(world);
   world->OverlapMultiByProfile(overlaps, avatar->GetActorLocation(), FQuat::Identity, _collisionProfile.Name, FCollisionShape::MakeSphere(_sphereRadius), params);

   const float addDistanceSqr = FMath::Square(_sphereRadius);
   for (const FOverlapResult& overlap : overlaps)
   {
      // redundantly check squared distance to center to make it behave uniformly with the removal logic
      AActor* overlapActor = overlap.GetActor();
      if (overlapActor == nullptr ||
         overlapActor->GetSquaredDistanceTo(avatar) > addDistanceSqr ||
         _trackedActors.Contains(overlapActor))
      {
         continue;
      }

      if (!_targetFilter(overlapActor))
      {
         continue;
      }

      _OnActorTracked(overlapActor);
      _trackedActors.Add(overlapActor);
   }

#if ENABLE_DRAW_DEBUG
   if (_drawDebug)
   {
      DrawDebugSphere(
         GetWorld(),
         avatar->GetActorLocation(),
         _sphereRadius,
         25,
         FColor(255, 0, 0),
         false,
         _pollInterval,
         0,
         1.5);

      DrawDebugSphere(
         GetWorld(),
         avatar->GetActorLocation(),
         _sphereRadius + _extraRemoveDistance,
         25,
         FColor(255, 0, 255),
         false,
         _pollInterval,
         0,
         0);
   }
#endif
}

void UAbilityTask_TrackActorsInSphere::_OnActorTracked(AActor* trackedActor)
{
   if (ShouldBroadcastAbilityTaskDelegates())
   {
      OnActorEnter.Broadcast(trackedActor);
   }
}

void UAbilityTask_TrackActorsInSphere::_OnActorRemoved(AActor* removedActor)
{
   if (ShouldBroadcastAbilityTaskDelegates())
   {
      OnActorLeave.Broadcast(removedActor);
   }
}



