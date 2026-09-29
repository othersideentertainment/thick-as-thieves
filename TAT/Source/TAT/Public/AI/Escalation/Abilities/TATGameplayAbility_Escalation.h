// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/Alertness/TATAlertnessComponent.h"
#include "AI/Escalation/TATEscalationComponent.h"

// ose
#include "Abilities/OSEGameplayAbility.h"

// ue
#include "CoreMinimal.h"

#include "TATGameplayAbility_Escalation.generated.h"


UCLASS()
class TAT_API UTATGameplayAbility_Escalation : public UOSEGameplayAbility
{
   GENERATED_BODY()

protected:
   UFUNCTION(BlueprintCallable)
   void SetEscalationState(ETATEscalationState newState);

   UFUNCTION(BlueprintCallable)
   UTATAlertnessComponent* GetTATAlertnessComponentFromActorInfo() const;

   UFUNCTION(BlueprintCallable)
   ATATAIController* GetTATAIController() const;
};
