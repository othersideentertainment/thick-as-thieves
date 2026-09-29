// (c) OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryTest_HasLineOfSight.generated.h"

/**
 * 
 */
UCLASS()
class OSEAI_API UEnvQueryTest_HasLineOfSight : public UEnvQueryTest
{
	GENERATED_BODY()
	public:
   UPROPERTY(EditDefaultsOnly, Category = "Behaviour")
   bool Hide;
   /** context: other end of line trace test */
   UPROPERTY(EditDefaultsOnly, Category = "Behaviour")
   TSubclassOf<UEnvQueryContext> Context;

public:
   UEnvQueryTest_HasLineOfSight(const FObjectInitializer& ObjectInitializer);

   // from EnvQueryTest
   virtual void RunTest(FEnvQueryInstance& queryInstance) const override;
   virtual FText GetDescriptionTitle() const override;
   virtual FText GetDescriptionDetails() const override;
};
