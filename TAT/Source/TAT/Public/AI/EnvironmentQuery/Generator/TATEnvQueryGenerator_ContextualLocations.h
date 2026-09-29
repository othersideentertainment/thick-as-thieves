// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameplayTagContainer.h"
#include "EnvironmentQuery/EnvQueryGenerator.h"

#include "TATEnvQueryGenerator_ContextualLocations.generated.h"

UCLASS()
class TAT_API UTATEnvQueryGenerator_ContextualLocations : public UEnvQueryGenerator
{
	GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, Category = "Generator", meta = (Categories = "AI.ContextualLocation"))
   FGameplayTag LocationType;

   UPROPERTY(EditDefaultsOnly, Category = "Generator")
   bool ConsiderUnclaimedLocationsOnly = true;

   UTATEnvQueryGenerator_ContextualLocations(const FObjectInitializer& objectInitializer);

   // from UEnvQueryGenerator
   virtual void GenerateItems(FEnvQueryInstance& queryInstance) const override;
   virtual FText GetDescriptionTitle() const override;
	
};
