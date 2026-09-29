// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Tasks/AbilityTask_WaitCommitCheck.h"
#include "AbilitySystemLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_WaitCommitCheck)

UAbilityTask_WaitCommitCheck::UAbilityTask_WaitCommitCheck(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   bTickingTask = true;
}

void UAbilityTask_WaitCommitCheck::TickTask(float deltaTime)
{
   if (Ability)
   {
      _CheckCommitSuccess();
   }
   else
   {
      ABILITY_LOG(Warning, TEXT("UAbilityTask_WaitCooldown ticked without a valid ability. ending."));
      EndTask();
   }
}

UAbilityTask_WaitCommitCheck* UAbilityTask_WaitCommitCheck::CreateWaitCommitCheck(UGameplayAbility* owningAbility)
{
   UAbilityTask_WaitCommitCheck* task = NewAbilityTask<UAbilityTask_WaitCommitCheck>(owningAbility);
   return task;
}

void UAbilityTask_WaitCommitCheck::Activate()
{
   _CheckCommitSuccess();
}

void UAbilityTask_WaitCommitCheck::_CheckCommitSuccess()
{
   if (Ability && Ability->CommitCheck(Ability->GetCurrentAbilitySpecHandle(), Ability->GetCurrentActorInfo(), Ability->GetCurrentActivationInfo()))
   {
      if (ShouldBroadcastAbilityTaskDelegates())
      {
         OnCommitCheckSuccess.Broadcast();
      }
      EndTask();
   }
}

