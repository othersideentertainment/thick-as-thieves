// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"

#include "EnvQueryTest_Attitude.generated.h"

enum class EOSETeamAttitude : uint8;

//
// Query Test to consider OSE team attitude of querier and actor items 
//
UCLASS()
class OSEAI_API UEnvQueryTest_Attitude : public UEnvQueryTest
{
	GENERATED_BODY()
public:
   UEnvQueryTest_Attitude();

   // from EnvQueryTest
   virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
   virtual FText GetDescriptionTitle() const override;
   virtual FText GetDescriptionDetails() const override;

   UPROPERTY(EditAnywhere, Category = "Attitude")
   EOSETeamAttitude desiredAttitude;

};
