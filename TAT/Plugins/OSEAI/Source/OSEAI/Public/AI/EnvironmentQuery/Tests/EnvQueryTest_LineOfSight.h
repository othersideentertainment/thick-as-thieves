// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_Pathfinding.h"
#include "EnvironmentQuery/EnvQueryContext.h"

#include "EnvQueryTest_LineOfSight.generated.h"

enum class EOSETeamAttitude : uint8;

//
// Query Test to check if position have Line of sight on target 
//
UCLASS()
class OSEAI_API UEnvQueryTest_LineOfSight : public UEnvQueryTest
{
	GENERATED_BODY()
public:
   UPROPERTY(EditDefaultsOnly, Category = "Behaviour")
   bool hide;
   /** context: other end of pathfinding test */
   UPROPERTY(EditDefaultsOnly, Category = Pathfinding)
   TSubclassOf<UEnvQueryContext> context;
   /** navigation filter to use in pathfinding */
   UPROPERTY(EditDefaultsOnly, Category = Pathfinding)
   TSubclassOf<UNavigationQueryFilter> filterClass;

public:
   UEnvQueryTest_LineOfSight(const FObjectInitializer& ObjectInitializer);

   // from EnvQueryTest
   virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
   virtual FText GetDescriptionTitle() const override;
   virtual FText GetDescriptionDetails() const override;
};
