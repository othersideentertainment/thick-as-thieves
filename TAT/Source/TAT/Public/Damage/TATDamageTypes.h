// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Damage/TATDamageTags.h"

// ue5
#include "CoreMinimal.h"
#include "ScalableFloat.h"

#include "TATDamageTypes.generated.h"

struct FTATScalableDamageWithType;

// A basic concrete damage with accompanying type
USTRUCT(BlueprintType)
struct TAT_API FTATDamageWithType
{
   GENERATED_BODY()

public:
   FTATDamageWithType() = default;
   explicit FTATDamageWithType(const FTATScalableDamageWithType& scalableDamage);

   FTATDamageWithType(float amount, FGameplayTag type) : DamageAmount(amount), DamageType(type)
   {}

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   float DamageAmount = 0;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, NoClear, meta = (Categories = "DamageType"))
   FGameplayTag DamageType = TAG_DamageType_Physical;
};

// A typed damage struct where the amount is a scalable float
// For use in design configuration where it might want to reference a curve table value
USTRUCT(BlueprintType)
struct TAT_API FTATScalableDamageWithType
{
   GENERATED_BODY()

public:

   UPROPERTY(EditAnywhere)
   FScalableFloat DamageAmount;

   UPROPERTY(EditAnywhere, BlueprintReadonly, NoClear, meta = (Categories = "DamageType"))
   FGameplayTag DamageType = TAG_DamageType_Physical;
};

