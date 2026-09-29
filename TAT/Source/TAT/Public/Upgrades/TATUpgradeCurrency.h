// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "TATUpgradeCurrency.generated.h"

USTRUCT(BlueprintType)
struct TAT_API FTATUpgradeCurrencyCost
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (Categories = "UpgradeCurrency"))
   FGameplayTag CurrencyTag;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta=(UIMin = 1, ClampMin = 1))
   int32 Amount = 1;
};
