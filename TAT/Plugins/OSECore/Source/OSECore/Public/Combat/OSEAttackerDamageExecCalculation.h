// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "GameplayEffectExecutionCalculation.h"

#include "OSEAttackerDamageExecCalculation.generated.h"

UCLASS()
class OSECORE_API UOSEAttackerDamageExecCalculation : public UGameplayEffectExecutionCalculation
{
   GENERATED_BODY()

public:
   UOSEAttackerDamageExecCalculation();

   // from UGameplayEffectExecutionCalculation
   virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
