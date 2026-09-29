// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/Tasks/AbilityTask_InterpControlRotationTo.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_InterpControlRotationTo)

UAbilityTask_InterpControlRotationTo::UAbilityTask_InterpControlRotationTo(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   bTickingTask = true;
}

UAbilityTask_InterpControlRotationTo* UAbilityTask_InterpControlRotationTo::InterpControlRotationTo(
   UGameplayAbility* owningAbility,
   const FRotator targetRotation,
   const float duration)
{
   UAbilityTask_InterpControlRotationTo* task = NewAbilityTask<UAbilityTask_InterpControlRotationTo>(owningAbility);
   task->_targetRotation = targetRotation;
   task->_duration = duration;
   return task;
}

void UAbilityTask_InterpControlRotationTo::Activate()
{
   if (IsLocallyControlled())
   {
      const FGameplayAbilityActorInfo* actorInfo = Ability->GetCurrentActorInfo();
      const APawn* pawn = Cast<APawn>(actorInfo->AvatarActor.Get());
      if (pawn != nullptr)
      {
         _owningController = pawn->GetController();
      }
      _startTime = GetWorld()->GetTimeSeconds();
      _endTime = _startTime + _duration;
      if (_owningController)
      {
         _currentRotation = _owningController->GetControlRotation();
         _owningController->SetIgnoreLookInput(true);
      }
   }
}

void UAbilityTask_InterpControlRotationTo::OnDestroy(const bool bInOwnerFinished)
{
   Super::OnDestroy(bInOwnerFinished);
   if (_owningController)
   {
      _owningController->SetIgnoreLookInput(false);
   }
}

void UAbilityTask_InterpControlRotationTo::TickTask(const float deltaTime)
{
   if (_owningController == nullptr)
   {
      EndTask();
      return;
   }
   const float currentTime = GetWorld()->GetTimeSeconds();
   const float alpha = FMath::GetMappedRangeValueClamped(
      FVector2D(_startTime, _endTime),
      FVector2D(0.f, 1.f),
      currentTime);
   _owningController->SetControlRotation(
      FMath::Lerp(
         _currentRotation, 
         _targetRotation,
         alpha)
   );
   if (alpha >= 1.f)
   {
      EndTask();
   }
}
