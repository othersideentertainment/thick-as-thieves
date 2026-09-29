// (c) 2021-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once
#include "AI/Escalation/TATEscalationComponent.h"
#include "Animation/Graph/OSEAnimAIAnimData.h"

#include "TATAnimAIData.generated.h"

USTRUCT(BlueprintType)
struct TAT_API FTATAIAnimData : public FOSEAnimAIAnimData
{
   GENERATED_BODY()
   
   UPROPERTY(EditAnywhere, BlueprintReadOnly) ETATEscalationState EscalationState = ETATEscalationState::Fresh;
   virtual void Update(const FOSEAnimActorInfo& actorInfo) override;
}; 
