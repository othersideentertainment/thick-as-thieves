// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Perception/AISenseEvent_VisualEvent.h"

#include "AI/Perception/AISense_VisualEvent.h"

#include "Perception/AISenseEvent.h"
#include "Perception/AISense.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AISenseEvent_VisualEvent)

FAISenseID UAISenseEvent_VisualEvent::GetSenseID() const
{
   return UAISense::GetSenseID<UAISense_VisualEvent>();
}

