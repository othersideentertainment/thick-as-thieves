// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Tasks/AbilityTask_WaitPositionChange.h"
#include "AbilitySystemLog.h"
#include "Math/Vector.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_WaitPositionChange)

UAbilityTask_WaitPositionChange::UAbilityTask_WaitPositionChange(const FObjectInitializer& ObjectInitializer)
   : Super(ObjectInitializer)
{
   bTickingTask = true;
}

void UAbilityTask_WaitPositionChange::TickTask(float DeltaTime)
{
   if (_cachedActor)
   {
      const FVector worldTargetPosition = _targetActor ?
         _targetActor->GetTransform().TransformPositionNoScale(_position) :
         _position;
      const FVector offset = (worldTargetPosition - _cachedActor->GetActorLocation());
      const float distanceSquared = offset.SizeSquared();

      if (distanceSquared > FMath::Square(_minimumDistance))
      {
         if (ShouldBroadcastAbilityTaskDelegates())
         {
            OnPositionChange.Broadcast();
         }
         EndTask();
      }
   }
   else
   {
      ABILITY_LOG(Warning, TEXT("UAbilityTask_WaitPositionChange ticked without a valid actor. ending."));
      EndTask();
   }
}

UAbilityTask_WaitPositionChange* UAbilityTask_WaitPositionChange::CreateWaitPositionChange(UGameplayAbility* owningAbility, FVector position, float minimumDistance)
{
   UAbilityTask_WaitPositionChange* task = NewAbilityTask<UAbilityTask_WaitPositionChange>(owningAbility);

   task->_position = position;
   task->_minimumDistance = minimumDistance;

   return task;
}

UAbilityTask_WaitPositionChange* UAbilityTask_WaitPositionChange::CreateWaitPositionChangeFromActor(UGameplayAbility* owningAbility, AActor* targetActor, FVector worldPosition, float minimumDistance)
{
   UAbilityTask_WaitPositionChange* task = NewAbilityTask<UAbilityTask_WaitPositionChange>(owningAbility);

   task->_targetActor = targetActor;
   task->_minimumDistance = minimumDistance;

   if (targetActor)
   {
      task->_position = targetActor->GetTransform().InverseTransformPositionNoScale(worldPosition);
   }
   else
   {
      // fall back to original behavior
      task->_position = worldPosition;
   }

   return task;
}

void UAbilityTask_WaitPositionChange::Activate()
{
   const FGameplayAbilityActorInfo* actorInfo = Ability->GetCurrentActorInfo();
   _cachedActor = actorInfo->AvatarActor.Get();
   SetWaitingOnAvatar();
}

