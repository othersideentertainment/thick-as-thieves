// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/TATContractObjectiveTriggerComponent.h"

// tat
#include "Quests/TATSharedObjectiveSubsystem.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATContractObjectiveTriggerComponent)

UTATContractObjectiveTriggerComponent::UTATContractObjectiveTriggerComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
}

void UTATContractObjectiveTriggerComponent::BeginPlay()
{
   Super::BeginPlay();

   if(GetOwner()->HasAuthority())
   {
      if(UTATSharedObjectiveSubsystem* objectiveSubsystem = GetWorld()->GetSubsystem<UTATSharedObjectiveSubsystem>())
      {
         constexpr ETATPlayerQuestSlot slot = ETATPlayerQuestSlot::Contract;
         if(objectiveSubsystem->WasSlotEverCompleted(slot))
         {
            AuthorityOnObjectiveComplete.Broadcast();
         }
         else
         {
            objectiveSubsystem->OnSlotEverCompleted(slot).AddWeakLambda(this, [this]()
            {
               AuthorityOnObjectiveComplete.Broadcast();
            });
         }
      }
   }
}

