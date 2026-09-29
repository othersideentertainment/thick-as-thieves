// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/SmartObjects/TATActionNodeComponent_Ambient.h"

// tat
#include "AI/Target/TATTargetingGroups.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATActionNodeComponent_Ambient)

UTATActionNodeComponent_Ambient::UTATActionNodeComponent_Ambient(const FObjectInitializer& objectInitializer) : Super(objectInitializer)
{
}

FGameplayTag UTATActionNodeComponent_Ambient::GetUtilityAITargetingGroup() const
{
   return TAG_AI_TargetingGroup_SmartObject_Ambient;
}

bool UTATActionNodeComponent_Ambient::HasMoreThanOneSlot() const
{
   return _numberOfSlots > 0;
}

bool UTATActionNodeComponent_Ambient::HasMoreThanOneSlotBeenOccupied() const
{
   return _occupiedSlots.Num() > 0;
}

void UTATActionNodeComponent_Ambient::_HandleSlotEvent(const FSmartObjectEventData& smartObjectEventData)
{
   if(smartObjectEventData.SlotHandle.IsValid() == false)
   {
      return;
   }
   if(smartObjectEventData.Reason == ESmartObjectChangeReason::OnReleased)
   {
      // On Released can be called when the Claim is released AND the occupation is ended.
      // So simply counting by incrementing / decrementing isn't going to work for us here. Instead use the slot handle
      // in an array and add / remove them from the array then use that as the "occupied" count.
      // The == operator on the slot handle is overriden and points to the smart object's mass entity index.
      if(_occupiedSlots.Remove(smartObjectEventData.SlotHandle) > 0)
      {
         OnSlotChanged.Broadcast(ESmartObjectChangeReason::OnReleased, smartObjectEventData.SlotHandle);  
      }
   }
   else if(smartObjectEventData.Reason == ESmartObjectChangeReason::OnOccupied)
   {
      _occupiedSlots.Add(smartObjectEventData.SlotHandle);
      OnSlotChanged.Broadcast(ESmartObjectChangeReason::OnOccupied, smartObjectEventData.SlotHandle);  
   }
}
