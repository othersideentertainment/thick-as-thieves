// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Combat/OSEDefenderDamageExecCalculation.h"

// ue4
#include "GameplayEffectExecutionCalculation.h"

#include "TATDefenderDamageExecCalculation.generated.h"

UCLASS()
class TAT_API UTATDefenderDamageExecCalculation : public UOSEDefenderDamageExecCalculation
{
   GENERATED_BODY()

public:
   UTATDefenderDamageExecCalculation();

   virtual float _ComputeDamageMagnitude(const FGameplayEffectCustomExecutionParameters& executionParams) const override;

   virtual void _HandleComputedDamage(float damage, const FGameplayEffectCustomExecutionParameters& executionParams, FGameplayEffectCustomExecutionOutput& outExecutionOutput) const;
};
