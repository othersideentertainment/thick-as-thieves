// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/SmartObjects/TATActionNodeComponent.h"

// tat

// ue
#include "SmartObjectSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATActionNodeComponent)

UTATActionNodeComponent::UTATActionNodeComponent(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
}


void UTATActionNodeComponent::BeginPlay()
{
   if(USmartObjectSubsystem* subsystem = USmartObjectSubsystem::GetCurrent(GetWorld()))
   {
      TArray<FSmartObjectSlotHandle> outSlots;
      subsystem->GetAllSlots(GetRegisteredHandle(), outSlots);
      _numberOfSlots = outSlots.Num();
      for (const auto& slot : outSlots)
      {
         if(FOnSmartObjectEvent* slotEvent = subsystem->GetSlotEventDelegate(slot))
         {
            slotEvent->AddUObject(this, &ThisClass::_HandleSlotEvent);
         }
      }
   }
   Super::BeginPlay();
}

void UTATActionNodeComponent::BeginDestroy()
{
   if(USmartObjectSubsystem* subsystem = USmartObjectSubsystem::GetCurrent(GetWorld()))
   {
      TArray<FSmartObjectSlotHandle> outSlots;
      subsystem->GetAllSlots(GetRegisteredHandle(), outSlots);
      for (const auto& slot : outSlots)
      {
         if(FOnSmartObjectEvent* slotEvent = subsystem->GetSlotEventDelegate(slot))
         {
            slotEvent->RemoveAll(this);
         }
      }
   }
   Super::BeginDestroy();
}

void UTATActionNodeComponent::_HandleSlotEvent(const FSmartObjectEventData& smartObjectEventData)
{
}
