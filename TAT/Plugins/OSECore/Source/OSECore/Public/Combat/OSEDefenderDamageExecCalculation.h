// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "GameplayEffectExecutionCalculation.h"
#include "NativeGameplayTags.h"

#include "OSEDefenderDamageExecCalculation.generated.h"


OSECORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Effect_ExecutionParam_Damage);

UCLASS()
class OSECORE_API UOSEDefenderDamageExecCalculation : public UGameplayEffectExecutionCalculation
{
   GENERATED_BODY()

public:
   UOSEDefenderDamageExecCalculation();

   // from UGameplayEffectExecutionCalculation
   virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;


protected:
   // not meant to be pure, side effects are allowed
   virtual float _ComputeDamageMagnitude(const FGameplayEffectCustomExecutionParameters& executionParams) const;

   virtual void _HandleComputedDamage(float damage, const FGameplayEffectCustomExecutionParameters& executionParams, FGameplayEffectCustomExecutionOutput& outExecutionOutput) const;
};
