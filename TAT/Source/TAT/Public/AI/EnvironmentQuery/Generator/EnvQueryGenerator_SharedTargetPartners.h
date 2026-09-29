// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "DataProviders/AIDataProvider.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvironmentQuery/EnvQueryGenerator.h"
#include "Templates/SubclassOf.h"

#include "EnvQueryGenerator_SharedTargetPartners.generated.h"

class UTATPrivateSpaceCharacterComponent;

// Code largely copied from UEnvQueryGenerator_ActorsOfClass, but acts on the actor class
// provided in the querier's partner class cached in its FTATSharedTarget.
UCLASS()
class TAT_API UEnvQueryGenerator_SharedTargetPartners : public UEnvQueryGenerator
{
   GENERATED_UCLASS_BODY()

   // If true, this will only returns actors of the shared target partner class within the SearchRadius of the SearchCenter context.  
   // If false, it will return ALL actors of the class type in the world.
   UPROPERTY(EditDefaultsOnly, Category = Generator)
   FAIDataProviderBoolValue GenerateItemsOnlyInRadius;
   
   // Max distance of path between point and context.  NOTE: Zero and negative values will never return any results if
   // UseRadius is true.  "Within" requires Distance < Radius.  Actors ON the circle (Distance == Radius) are excluded.
   UPROPERTY(EditDefaultsOnly, Category = Generator)
   FAIDataProviderFloatValue SearchRadius;
   
   // Context specifying where to perform the radius check from.
   UPROPERTY(EditAnywhere, Category = Generator)
   TSubclassOf<UEnvQueryContext> SearchCenter;

   // from UEnvQueryGenerator
   virtual void GenerateItems(FEnvQueryInstance& queryInstance) const override;

   // from UEnvQueryNode
   virtual FText GetDescriptionTitle() const override;
   virtual FText GetDescriptionDetails() const override;
	
};
