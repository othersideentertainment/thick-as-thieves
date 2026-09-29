// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryGenerator.h"
#include "EnvQueryGenerator_DreadSpector.generated.h"


UCLASS()
class TAT_API UEnvQueryGenerator_DreadSpector : public UEnvQueryGenerator
{
   GENERATED_BODY()

public:
   UEnvQueryGenerator_DreadSpector(const FObjectInitializer& objectInitializer);
   virtual void GenerateItems(FEnvQueryInstance& queryInstance) const override;
   virtual FText GetDescriptionTitle() const override;
};
