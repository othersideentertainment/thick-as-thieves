// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "TATAISenseConfig_Sight.h"
#include "AI/Perception/OSEAISense_Sight.h"
#include "TATAISense_Sight.generated.h"

UCLASS()
class TAT_API UTATAISense_Sight : public UOSEAISense_Sight
{
   GENERATED_BODY()
protected:
   static ETATEscalationState GetEscalationStateForActor(const AActor* actor);
   virtual const FDigestedSightProperties& _SetupDigestedPropertiesForListener(const UAIPerceptionComponent& perceptionComponent) override;
};
