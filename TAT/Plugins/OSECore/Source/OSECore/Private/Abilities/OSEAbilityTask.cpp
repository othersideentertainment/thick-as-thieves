// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/OSEAbilityTask.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAbilityTask)


// Constructor
UOSEAbilityTask::UOSEAbilityTask(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
}

// Tick function for this task, if bTickingTask == true
void UOSEAbilityTask::TickTask(float deltaTime)
{
   Super::TickTask(deltaTime);
}

// Called to trigger the actual task once the delegates have been set up
void UOSEAbilityTask::Activate()
{
   Super::Activate();
};

// End and CleanUp the task - may be called by the task itself or by the task owner if the owner is ending.
void UOSEAbilityTask::OnDestroy(bool inOwnerFinished)
{
   Super::OnDestroy(inOwnerFinished);
}

