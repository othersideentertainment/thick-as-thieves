// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Perception/TATAISenseEvent_Hearing.h"

// tat
#include "AI/Perception/TATAISense_Hearing.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAISenseEvent_Hearing)

//----------------------------------------------------------------------//
// UTATAISenseEvent_Hearing
//----------------------------------------------------------------------//

UTATAISenseEvent_Hearing::UTATAISenseEvent_Hearing(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{

}

FAISenseID UTATAISenseEvent_Hearing::GetSenseID() const
{
   return UAISense::GetSenseID<UTATAISense_Hearing>();
}

