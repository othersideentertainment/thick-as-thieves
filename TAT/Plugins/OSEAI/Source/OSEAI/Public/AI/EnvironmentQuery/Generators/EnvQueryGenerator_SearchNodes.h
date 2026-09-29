// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose

// ue4
#include "DataProviders/AIDataProvider.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvironmentQuery/EnvQueryGenerator.h"

#include "EnvQueryGenerator_SearchNodes.generated.h"

UCLASS(meta = (DisplayName = "Search Nodes"))
class OSEAI_API UEnvQueryGenerator_SearchNodes : public UEnvQueryGenerator
{
   GENERATED_BODY()

   UEnvQueryGenerator_SearchNodes(const FObjectInitializer& objectInitializer);

   /// If true, this will only returns actors of the specified class within the SearchRadius of the SearchCenter context.
   /// If false, it will return ALL actors of the specified class in the world.
   UPROPERTY(EditDefaultsOnly, Category = "Generator")
   FAIDataProviderBoolValue GenerateOnlyActorsInRadius;

   /// Max distance of path between point and context.  NOTE: Zero and negative values will never return any results if
   /// UseRadius is true.  "Within" requires Distance < Radius.  Actors ON the circle (Distance == Radius) are excluded.
   UPROPERTY(EditDefaultsOnly, Category = "Generator")
   FAIDataProviderFloatValue SearchRadius;

   /// Context
   UPROPERTY(EditAnywhere, Category = "Generator")
   TSubclassOf<UEnvQueryContext> SearchCenter;

   // from UEnvQueryGenerator
   virtual void GenerateItems(FEnvQueryInstance& queryInstance) const override;
   virtual FText GetDescriptionTitle() const override;
   virtual FText GetDescriptionDetails() const override;
};
