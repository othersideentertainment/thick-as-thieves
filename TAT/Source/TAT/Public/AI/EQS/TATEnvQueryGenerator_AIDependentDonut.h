// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_Donut.h"
#include "TATEnvQueryGenerator_AIDependentDonut.generated.h"

UCLASS()
class TAT_API UTATEnvQueryGenerator_AIDependentDonut : public UEnvQueryGenerator_Donut
{
   GENERATED_BODY()
   
public:
   virtual void ProjectAndFilterNavPoints(TArray<FNavLocation>& points, FEnvQueryInstance& queryInstance) const override;
};
