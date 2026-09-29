// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/Tasks/AbilityTask_MaintainViewTarget.h"

// ue4
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_MaintainViewTarget)

DEFINE_LOG_CATEGORY_STATIC(LogAbilityTask_MaintainViewTarget, Log, All);

namespace AbilityTaskHelpers
{
   void GetActorViewLocationAndRotation(AActor* actor, FVector& viewPos, FRotator& viewRot)
   {
      if (actor == nullptr)
      {
         return;
      }
      if (ACharacter* character = Cast<ACharacter>(actor))
      {
         if (APlayerController* pc = Cast<APlayerController>(character->GetController()))
         {
            pc->GetPlayerViewPoint(viewPos, viewRot);
            return;
         }
      }
      actor->GetActorEyesViewPoint(viewPos, viewRot);
   }
}

// static
UTATAbilityTask_MaintainViewTarget* UTATAbilityTask_MaintainViewTarget::MaintainViewTarget(UGameplayAbility* owningAbility, AActor* targetActor,
   const FTATLineOfSightTraceParams& traceParams, float pollInterval, float angleDegThreshold, float distanceThreshold, float maxDurationSeconds, bool drawDebug)
{
   UTATAbilityTask_MaintainViewTarget* task = NewAbilityTask<UTATAbilityTask_MaintainViewTarget>(owningAbility);
   task->_targetActor = targetActor;
   task->_traceParams = traceParams;
   task->_angleDegThreshold = angleDegThreshold;
   task->_distanceThreshold = distanceThreshold;
   task->_maxDurationSeconds = maxDurationSeconds;
   task->_pollInterval = pollInterval;
   task->_drawDebug = drawDebug;
   return task;
}

void UTATAbilityTask_MaintainViewTarget::Activate()
{
   Super::Activate();

   if (_targetActor == nullptr)
   {
      UE_LOG(LogAbilityTask_MaintainViewTarget, Warning, TEXT("Ability task was activated without a valid view target. The view target event will never fire."));
      return;
   }

   UWorld* world = GetWorld();

   _startTimeSeconds = (world != nullptr) ? world->GetTimeSeconds() : -1.0;

   _Update();

   if (world != nullptr)
   {
      world->GetTimerManager().SetTimer(_timerHandle, this, &UTATAbilityTask_MaintainViewTarget::_Update, _pollInterval, true);
   }
}

void UTATAbilityTask_MaintainViewTarget::OnDestroy(bool abilityIsEnding)
{
   if (_timerHandle.IsValid())
   {
      if (UWorld* world = GetWorld())
      {
         world->GetTimerManager().ClearTimer(_timerHandle);
      }
   }

   Super::OnDestroy(abilityIsEnding);
}

void UTATAbilityTask_MaintainViewTarget::_OnMaxDurationReached(bool hasLineOfSightToTarget, float targetAngleDeg, float targetDistance, float elapsedTimeSeconds)
{
   UWorld* world = GetWorld();
   if (world != nullptr)
   {
      world->GetTimerManager().ClearTimer(_timerHandle);
   }

   EndTask();

   OnMaxDurationReached.Broadcast(hasLineOfSightToTarget, targetAngleDeg, targetDistance, elapsedTimeSeconds);
}

void UTATAbilityTask_MaintainViewTarget::_Update()
{
   UAbilitySystemComponent* asc = AbilitySystemComponent.Get();
   if (asc == nullptr || !asc->AbilityActorInfo.IsValid())
   {
      return;
   }

   if (!ensure(_targetActor))
   {
      return;
   }

   AActor* sourceActor = asc->AbilityActorInfo->AvatarActor.Get();
   if (sourceActor == nullptr)
   {
      return;
   }

   const float elapsedTimeSeconds = static_cast<float>(FMath::Max(0.0, asc->GetWorld()->GetTimeSeconds() - _startTimeSeconds));

   const bool shouldCancelTask = _maxDurationSeconds > 0 && elapsedTimeSeconds >= _maxDurationSeconds;

   FHitResult hitResult{};
   const bool hasLineOfSight = UTATToolFunctionLibrary::PerformLineOfSightTrace(hitResult, sourceActor, _targetActor, _traceParams, _drawDebug, _pollInterval);

   // Get the angle to the target using a dot product
   FVector sourceLocation;
   FRotator sourceRotation;
   AbilityTaskHelpers::GetActorViewLocationAndRotation(sourceActor, sourceLocation, sourceRotation);
   const FVector sourceFwd = sourceRotation.Vector().GetSafeNormal();

   float targetAngleDeg = FLT_MAX;
   float targetDistance = FLT_MAX;
   TArray<FBox, TInlineAllocator<2>> targetBounds;
   UTATToolFunctionLibrary::GetTargetCharacterBounds(_targetActor, targetBounds);
   check(targetBounds.Num() > 0);
   for (const FBox& bounds : targetBounds)
   {
      // get the view angle to the target
      const FVector targetLocation = bounds.GetCenter();
      const FVector targetDir = (targetLocation - sourceLocation).GetSafeNormal();
      const float angleDeg = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(sourceFwd, targetDir)));
      if (angleDeg < targetAngleDeg)
      {
         targetAngleDeg = angleDeg;
      }

      // get the distance to the target
      const float distance = FVector::Distance(sourceLocation, targetLocation);
      if (distance < targetDistance)
      {
         targetDistance = distance;
      }
   }
   check(targetAngleDeg != FLT_MAX);
   check(targetDistance != FLT_MAX);

   bool fireUpdateEvent = false;

   if (!_isFirstUpdate)
   {
      // Fire an update if the line of sight, angle, or distance changed from the last update.
      // For the angle and distance, only fire the event if the value changed enough to justify it.
      if (_prevHasLineOfSight != hasLineOfSight)
      {
         _prevHasLineOfSight = hasLineOfSight;
         fireUpdateEvent = true;
      }

      if (FMath::Abs(targetAngleDeg - _prevAngleDeg) >= _angleDegThreshold)
      {
         _prevAngleDeg = targetAngleDeg;
         fireUpdateEvent = true;
      }

      if (FMath::Abs(targetDistance - _prevDistance) >= _distanceThreshold)
      {
         _prevDistance = targetDistance;
         fireUpdateEvent = true;
      }
   }
   else
   {
      fireUpdateEvent = true;
      _prevHasLineOfSight = hasLineOfSight;
      _prevAngleDeg = targetAngleDeg;
      _prevDistance = targetDistance;
   }

   if (fireUpdateEvent || shouldCancelTask)
   {
      OnViewTargetUpdate.Broadcast(hasLineOfSight, targetAngleDeg, targetDistance, elapsedTimeSeconds);
   }

   _isFirstUpdate = false;

   if (shouldCancelTask)
   {
      _OnMaxDurationReached(hasLineOfSight, targetAngleDeg, targetDistance, elapsedTimeSeconds);
   }
}
