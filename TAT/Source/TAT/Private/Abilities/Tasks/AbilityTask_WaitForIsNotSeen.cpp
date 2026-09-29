// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/Tasks/AbilityTask_WaitForIsNotSeen.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_WaitForIsNotSeen)

DEFINE_LOG_CATEGORY_STATIC(LogTATAbilityTaskWaitForIsNotSeen, Log, All);

UAbilityTask_WaitForIsNotSeen::UAbilityTask_WaitForIsNotSeen(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   bTickingTask = true;
}

UAbilityTask_WaitForIsNotSeen* UAbilityTask_WaitForIsNotSeen::WaitForIsNotSeen(UGameplayAbility* owningAbility, ATATCharacter* inSeenCharacter, float inTimeoutTime)
{
   UAbilityTask_WaitForIsNotSeen* task = NewAbilityTask<UAbilityTask_WaitForIsNotSeen>(owningAbility);
   
   ensure(inSeenCharacter);
   task->seenCharacter = inSeenCharacter;
   task->timeoutTime = inTimeoutTime;

   return task;
}

void UAbilityTask_WaitForIsNotSeen::TickTask(float deltaTime)
{
   if (TaskState != EGameplayTaskState::Active)
      return;

   if (!seenCharacter)
   {
      UE_LOG(LogTATAbilityTaskWaitForIsNotSeen, Error, TEXT("SeenCharacter was null for UAbilityTask_WaitForIsNotSeen!"));
      EndTask();
      return;
   }

   if (!seenCharacter->HasAuthority())
   {
      UE_LOG(LogTATAbilityTaskWaitForIsNotSeen, Error, TEXT("SeenCharacter '%s' is not authority for UAbilityTask_WaitForIsNotSeen!"), *seenCharacter->GetName());
      EndTask();
      return;
   }

   timeoutTime -= deltaTime;

   if (timeoutTime < 0.f)
   {
      OnTimeout.Broadcast();
      EndTask();
   }

   bool foundHostileViewer = false;
   for (AActor* viewingActor : seenCharacter->AuthorityGetViewingActors())
   {
      EOSETeamAttitude teamAttitude = UOSETeamFunctionLibrary::GetTeamAttitude(viewingActor, seenCharacter);
      if (teamAttitude == EOSETeamAttitude::Hostile)
      {
         foundHostileViewer = true;
         break;
      }
   }

   if (!foundHostileViewer)
   {
      OnNotSeen.Broadcast();
      EndTask();
   }
}

