// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"

#include "TATDamageThresholdExecution.generated.h"


USTRUCT()
struct TAT_API FTATThresholdExecutionEntry
{
   GENERATED_BODY()

   /// The minimum damage magnitude for this threshold
   UPROPERTY(EditDefaultsOnly)
   FScalableFloat MinMagnitude = 0;

   /// Effect to grant if damage in in this threshold
   /// It will be applied with the level of the index of this threshold (e.g. 0,1,2)
   UPROPERTY(EditDefaultsOnly)
   TSubclassOf<UGameplayEffect> EffectToGrant = nullptr;
};

/// An execution that applies a gameplay effect depending on the magnitude of damage in prior executions
UCLASS(Abstract, Blueprintable)
class TAT_API UTATDamageThresholdExecution : public UGameplayEffectExecutionCalculation
{
   GENERATED_BODY()
   
public:

   UTATDamageThresholdExecution();

   // from UGameplayEffectExecutionCalculation
   virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& executionParams, OUT FGameplayEffectCustomExecutionOutput& outExecutionOutput) const override;
   
   UPROPERTY(EditDefaultsOnly)
   TArray<FGameplayAttribute> Attributes;

   UPROPERTY(EditDefaultsOnly)
   TArray<FTATThresholdExecutionEntry> Thresholds;

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

private:
   const FGameplayEffectModifiedAttribute* _FindModifiedAttribute(const FGameplayEffectSpec& spec) const;
};
