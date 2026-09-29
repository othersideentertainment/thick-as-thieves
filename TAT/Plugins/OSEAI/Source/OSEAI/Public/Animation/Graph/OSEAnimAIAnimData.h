// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// OSE
#include "AI/Alertness/AlertnessEnums.h"
#include "Animation/Graph/OSEAnimData.h"

#include "OSEAnimAIAnimData.generated.h"

USTRUCT(BlueprintType)
struct OSEAI_API FOSEAnimAIAnimData : public FOSEBaseAnimData
{
   GENERATED_BODY()
   
   // Alertness (AI) data
   UPROPERTY(EditAnywhere, BlueprintReadOnly) EAlertnessLevel AlertnessLevel = EAlertnessLevel::Neutral;

   virtual void Update(const FOSEAnimActorInfo& actorInfo) override;
};
