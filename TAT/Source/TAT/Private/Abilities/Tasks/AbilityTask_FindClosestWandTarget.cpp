// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Tasks/AbilityTask_FindClosestWandTarget.h"

// ose
#include "AbilitySystemComponent.h"
#include "Abilities/TargetActors/ConeTargetHelpers.h"
#include "Character/OSECharacterBase.h"

#include "Abilities/GameplayAbility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_FindClosestWandTarget)


UAbilityTask_FindClosestWandTarget::UAbilityTask_FindClosestWandTarget()
{

}


// static
UAbilityTask_FindClosestWandTarget* UAbilityTask_FindClosestWandTarget::FindClosestWandTarget(
   UGameplayAbility* owningAbility,
   AActor* actor,
   float maxRange,
   float maxDegreesFromForward,
   float pollFrequency,
   FCollisionProfileName traceProfile,
   const FGameplayTagContainer& blockedTags,
   bool debugDraw /*= false*/
)
{
   UAbilityTask_FindClosestWandTarget* task = NewAbilityTask<UAbilityTask_FindClosestWandTarget>(owningAbility);
   
   AActor* owningActor = actor ? actor : owningAbility->GetAvatarActorFromActorInfo();

   task->_sourceActor = owningActor;
   task->_maxDegreesFromForward = maxDegreesFromForward;
   task->_pollFrequency = pollFrequency;
   task->_traceProfile = traceProfile;
   task->_blockedTags = blockedTags;
   task->_debugDraw = debugDraw;
   return task;
}

void UAbilityTask_FindClosestWandTarget::Activate()
{
   Super::Activate();

   if (UWorld* world = GetWorld())
   {
      world->GetTimerManager().SetTimer(
         _timerHandle,
         this,
         &UAbilityTask_FindClosestWandTarget::_RefreshTarget,
         _pollFrequency,
         true);
   }
}

void UAbilityTask_FindClosestWandTarget::OnDestroy(bool abilityIsEnding)
{
   if (UWorld* world = GetWorld())
   {
      world->GetTimerManager().ClearTimer(_timerHandle);
   }

   Super::OnDestroy(abilityIsEnding);
}

void UAbilityTask_FindClosestWandTarget::_RefreshTarget()
{
   ConeTargetHelpers::FConeTraceParams params;
   params.SourceActor = _sourceActor;
   params.MaxRange = _maxRange;
   params.HalfAngle = _maxDegreesFromForward;
   params.LineOfSightProfile = _traceProfile;
   params.ObjectQueryParams = FCollisionObjectQueryParams(ECC_Pawn);
   params.DrawDebug = _debugDraw;
   params.RankCriteria = ConeTargetHelpers::EConeTraceRankCriteria::ShortestDistanceToTraceLine;
   auto filter = [&](const AActor* possibleTarget)
   {
      if (const AOSECharacterBase* character = Cast<const AOSECharacterBase>(possibleTarget))
      {
         if (const UAbilitySystemComponent* asc = character->GetAbilitySystemComponent())
         {
            return !asc->HasAnyMatchingGameplayTags(_blockedTags);
         }
      }
      return false;
   };

   ConeTargetHelpers::FConeTraceResult result = ConeTargetHelpers::DoConeTrace(params, filter);
   if (result.FoundActor != _currentTarget)
   {
      _currentTarget = result.FoundActor;
      OnClosestWandTargetChanged.Broadcast(_currentTarget);
   }
}

