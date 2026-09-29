// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/Tasks/AbilityTask_PerformJump.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_PerformJump)


// Constructor
UAbilityTask_PerformJump::UAbilityTask_PerformJump(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
   , _duration(KINDA_SMALL_NUMBER)
   , _elapsed(0.0f)
   , _initiatedJump(false)
   , _useMaxDuration(false)
{
   bTickingTask = true;
}

// Perform a jump
UAbilityTask_PerformJump* UAbilityTask_PerformJump::PerformJump(UGameplayAbility* owningAbility, float duration)
{
   UAbilityTask_PerformJump* myTask = NewAbilityTask<UAbilityTask_PerformJump>(owningAbility);
   myTask->_duration = FMath::Max(duration, KINDA_SMALL_NUMBER); // Avoid negative or divide-by-zero cases
   myTask->_elapsed = 0.0f;
   myTask->_initiatedJump = false;
   myTask->_useMaxDuration = false;
   return myTask;
}

// Perform a jump that will use the maximum hold time
UAbilityTask_PerformJump* UAbilityTask_PerformJump::PerformJumpMaxHold(UGameplayAbility* owningAbility)
{
   const float kDefaultDuration = 0.05f;

   UAbilityTask_PerformJump* myTask = PerformJump(owningAbility, kDefaultDuration);
   myTask->_useMaxDuration = true;
   return myTask;
}

// Called to trigger the actual task once the delegates have been set up
void UAbilityTask_PerformJump::Activate()
{
   Super::Activate();

   // Cancel if we don't have a character, movement component, or aren't allowed to jump
   if (!_traversalCharacter || !_traversalMovement || !_traversalMovement->IsJumpAllowed())
   {
      if (ShouldBroadcastAbilityTaskDelegates())
      {
         OnCancelled.Broadcast(_elapsed);
      }

      EndTask();
      return;
   }

   if (_useMaxDuration)
   {
      _duration = FMath::Max(_duration, _traversalCharacter->GetJumpMaxHoldTime());
   }

   // We have everything we need to jump, so start it
   _initiatedJump = true;
   _traversalCharacter->Jump();
};

// End and CleanUp the task - may be called by the task itself or by the task owner if the owner is ending.
void UAbilityTask_PerformJump::OnDestroy(bool inOwnerFinished)
{
   if (_initiatedJump)
   {
      _initiatedJump = false;

      if (_traversalCharacter != nullptr)
         _traversalCharacter->StopJumping();
   }

   Super::OnDestroy(inOwnerFinished);
}

// Tick function for this task, if bTickingTask == true
void UAbilityTask_PerformJump::TickTask(float deltaTime)
{
   _elapsed += deltaTime;

   if (_elapsed >= _duration)
   {
      // Use a timer to stop jumping next tick (this may actually happen this tick, which is fine)
      GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UAbilityTask_PerformJump::OnJumpFinished);
   }
}

// Called to stop the mantle
void UAbilityTask_PerformJump::OnJumpFinished()
{
   _initiatedJump = false;
   _traversalCharacter->StopJumping();

   if (ShouldBroadcastAbilityTaskDelegates())
   {
      OnFinished.Broadcast(_elapsed);
   }

   EndTask();
}

