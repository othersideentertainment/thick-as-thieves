// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryGenerator.h"

#include "EnvQueryGenerator_ActorKnowledge.generated.h"

UCLASS()
class TAT_API UEnvQueryGenerator_ActorKnowledge : public UEnvQueryGenerator
{
   GENERATED_BODY()
public:

   UEnvQueryGenerator_ActorKnowledge(const FObjectInitializer& objectInitializer);

   // from UEnvQueryGenerator
   virtual void GenerateItems(FEnvQueryInstance& queryInstance) const override;
   virtual FText GetDescriptionTitle() const override;

   // If true, items will only be generated for known actors that have been fully identified.
   UPROPERTY(EditDefaultsOnly, Category = "Generator")
   bool RequireActorIdentified = false;
};
