// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Tasks/AbilityTask_FindSphereOverlapActors.h"
#include "Character/OSECharacterBase.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_FindSphereOverlapActors)


UAbilityTask_FindSphereOverlapActors::UAbilityTask_FindSphereOverlapActors(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
#if ENABLE_DRAW_DEBUG
   bTickingTask = true;
#endif
}

UAbilityTask_FindSphereOverlapActors* UAbilityTask_FindSphereOverlapActors::FindSphereOverlapActors(UGameplayAbility* owningAbility, AActor* actor, float sphereRadius, float timeBetweenActions,
   TArray<TEnumAsByte<EObjectTypeQuery>> objectTypes, bool drawDebugSphere)
{
   UAbilityTask_FindSphereOverlapActors* task = NewAbilityTask<UAbilityTask_FindSphereOverlapActors>(owningAbility);
   task->_actor = actor;
   task->_ignore = { actor };
   task->_sphereRadius = sphereRadius;
   task->_timeBetweenActions = timeBetweenActions;
   task->_overlapObjectTypes = objectTypes;
   task->_drawDebugSphere = drawDebugSphere;
   return task;
}

void UAbilityTask_FindSphereOverlapActors::TickTask(float deltaTime)
{
#if ENABLE_DRAW_DEBUG
   if (_drawDebugSphere && _actor)
   {
      DrawDebugSphere(
         GetWorld(),
         _actor->GetActorLocation(),
         _sphereRadius,
         25,
         FColor(255, 0, 0),
         false,
         -1.f,
         0,
         1.5);
   }
#endif
}

void UAbilityTask_FindSphereOverlapActors::Activate()
{
   if (!_actor)
   {
      const FGameplayAbilityActorInfo* actorInfo = Ability->GetCurrentActorInfo();
      _actor = actorInfo->AvatarActor.Get();
      if (_actor)
         _ignore = { _actor };
   }

   if (UWorld* world = GetWorld())
   {
      world->GetTimerManager().SetTimer(
         _timerHandle_FindActors,
         this,
         &UAbilityTask_FindSphereOverlapActors::_FindActors,
         _timeBetweenActions,
         true);
   }
}

void UAbilityTask_FindSphereOverlapActors::OnDestroy(bool abilityIsEnding)
{
   if (UWorld* world = GetWorld())
   {
      world->GetTimerManager().ClearTimer(_timerHandle_FindActors);
   }

   Super::OnDestroy(abilityIsEnding);
}

void UAbilityTask_FindSphereOverlapActors::_FindActors()
{
   // 8/3/21 -- I think _actor is null, sometimes?  Let's avoid crashing, at least.
   ensure(_actor);
   if (_actor)
   {
      UKismetSystemLibrary::SphereOverlapActors(
         _actor,
         _actor->GetActorLocation(),
         _sphereRadius,
         _overlapObjectTypes,
         nullptr,
         _ignore,
         _outActors);
   }

   FoundActors.Broadcast(_outActors);
}

void UAbilityTask_FindSphereOverlapActors::CancelTask()
{
   EndTask();
}

