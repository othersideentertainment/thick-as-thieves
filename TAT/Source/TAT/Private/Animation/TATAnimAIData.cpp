// (c) 2021-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Animation/TATAnimAIData.h"

// tat
#include "Character/TATCharacterAIBase.h"

// ose
#include "Animation/Graph/OSEAnimActorInfo.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAnimAIData)

void FTATAIAnimData::Update(const FOSEAnimActorInfo& actorInfo)
{
   if(const ATATCharacterAIBase* aiBase = Cast<ATATCharacterAIBase>(actorInfo.PawnOwner.Get()))
   {
      EscalationState = aiBase->GetCurrentEscalationState();
   }
   Super::Update(actorInfo);
}
