// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "SignificanceSettings.generated.h"

USTRUCT()
struct FSignificanceThresholds
{
   GENERATED_BODY()
   
   UPROPERTY(EditDefaultsOnly)
   float Significance { 1.f };
   UPROPERTY(EditDefaultsOnly)
   float MaxDistance { 1000.f };
};

USTRUCT()
struct FSignificanceSettings
{
   GENERATED_BODY()
   
   UPROPERTY(EditDefaultsOnly)
   TArray<FSignificanceThresholds> SignificanceThresholds;
};
