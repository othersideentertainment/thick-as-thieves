// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Tasks/AbilityTask_Awareness.h"

#include "AI/Alertness/OSEAlertnessInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_Awareness)

void UEventTimelineActionRaiseAlertnessLevelToAtLeast::EvaluateAction(AActor* actor, AActor* otherActor) const
{
   if (actor->HasAuthority())
   {
      if (const IOSEAlertnessInterface* alertnessInterface = Cast<IOSEAlertnessInterface>(actor))
      {
         if (UOSEAlertnessComponent* alertnessComp = alertnessInterface->GetAlertnessComponent())
         {
            alertnessComp->AuthorityRaiseAlertnessLevelToAtLeast(AlertnessLevel);
         }
      }
   }
}

void UEventTimelineActionLowerAlertnessLevelToAtMost::EvaluateAction(AActor* actor, AActor* otherActor) const
{
   if (actor->HasAuthority())
   {
      if (const IOSEAlertnessInterface* alertnessInterface = Cast<IOSEAlertnessInterface>(actor))
      {
         if (UOSEAlertnessComponent* alertnessComp = alertnessInterface->GetAlertnessComponent())
         {
            alertnessComp->AuthorityLowerAlertnessLevelToAtMost(AlertnessLevel);
         }
      }
   }
}
