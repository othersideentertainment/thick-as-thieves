// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Traps/TATTrapDetectorComponent.h"

// tat
#include "Traps/TATTrapActionInterface.h"
#include "Interactables/TATSecurityLockdownComponent.h" //< temp
#include "AI/Squad/TATSquadAlarmStation.h" //< temp

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTrapDetectorComponent)

void UTATTrapDetectorComponent::TriggerTrapActions(AActor* optionalTarget)
{
   for (AActor* trapToTrigger : TrapActionsToTrigger)
   {
      if(trapToTrigger == nullptr)
         continue;
      if(trapToTrigger->Implements<UTATTrapActionInterface>())
      {
         ITATTrapActionInterface::Execute_TriggerActionFromTrap(trapToTrigger, optionalTarget);
      }
   }
}

void UTATTrapDetectorComponent::TEMP_MigrateFrom(UTATTrapDetectorComponent* other)
{
   if (other)
   {
      // Strips null and dup while here
      for (AActor* actor : other->TrapActionsToTrigger)
      {
         if (actor)
         {
            TrapActionsToTrigger.AddUnique(actor);
         }
      }
   }
}

void UTATTrapDetectorComponent::TEMP_MigrateFromLockdown(class UTATSecurityLockdownComponent* lockdown)
{
   if (lockdown)
   {
      for (ATATSquadAlarmStation* alarm : lockdown->GetConnectedAlarms())
      {
         if (alarm)
         {
            TrapActionsToTrigger.AddUnique(alarm);
         }
      }
   }
}
