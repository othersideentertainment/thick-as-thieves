// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// ose ai
#include "Animation/Graph/OSEAnimAIAnimData.h"
#include "AI/Alertness/OSEAlertnessInterface.h"

// ose 
#include "Animation/Graph/OSEAnimActorInfo.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAnimAIAnimData)

void FOSEAnimAIAnimData::Update(const FOSEAnimActorInfo& actorInfo)
{
   if (const auto alertnessInterface = Cast<IOSEAlertnessInterface>(actorInfo.PawnOwner.Get()))
   {
      AlertnessLevel = alertnessInterface->GetAlertnessLevel();
   }
}
