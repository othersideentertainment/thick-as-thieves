// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_Pathfinding.h"
#include "TATEnvQueryTest_AIDependentPathfinding.generated.h"

UCLASS()
class TAT_API UTATEnvQueryTest_AIDependentPathfinding : public UEnvQueryTest_Pathfinding
{
   GENERATED_BODY()
   
public:
   virtual void RunTest(FEnvQueryInstance& queryInstance) const override;
   virtual FText GetDescriptionTitle() const override;
};
