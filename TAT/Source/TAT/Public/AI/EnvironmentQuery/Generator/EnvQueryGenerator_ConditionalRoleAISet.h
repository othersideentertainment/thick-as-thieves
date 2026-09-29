// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryGenerator.h"

#include "EnvQueryGenerator_ConditionalRoleAISet.generated.h"

UCLASS()
class TAT_API UEnvQueryGenerator_ConditionalRoleAISet : public UEnvQueryGenerator
{
   GENERATED_BODY()
public:

   UEnvQueryGenerator_ConditionalRoleAISet(const FObjectInitializer& objectInitializer);

   // from UEnvQueryGenerator
   virtual void GenerateItems(FEnvQueryInstance& queryInstance) const override;
   virtual FText GetDescriptionTitle() const override;
};
