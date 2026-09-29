// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/Tasks/AbilityTask_AddMovementInput.h"

// ue4
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_AddMovementInput)


UAbilityTask_AddMovementInput::UAbilityTask_AddMovementInput(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   bTickingTask = true;
}


UAbilityTask_AddMovementInput* UAbilityTask_AddMovementInput::ApplyContinuousMovementInput(UGameplayAbility* owningAbility, FVector localDirection, float magnitude)
{
   UAbilityTask_AddMovementInput* task = NewAbilityTask<UAbilityTask_AddMovementInput>(owningAbility);

   task->_localDirection = localDirection;
   task->_magnitude = magnitude;

   return task;
}

void UAbilityTask_AddMovementInput::TickTask(float deltaTime)
{
   if (_pawn)
   {
      const FVector worldDirection = _pawn->GetTransform().TransformVectorNoScale(_localDirection);
      _pawn->AddMovementInput(worldDirection, _magnitude);
   }
   else
   {
      EndTask();
   }
}

void UAbilityTask_AddMovementInput::Activate()
{
   if (IsLocallyControlled())
   {
      const FGameplayAbilityActorInfo* actorInfo = Ability->GetCurrentActorInfo();
      _pawn = Cast<APawn>(actorInfo->AvatarActor.Get());
   }

   SetWaitingOnAvatar();
}

