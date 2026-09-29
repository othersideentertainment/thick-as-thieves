// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "OSESense.h"
#include "OSESense_Prediction.generated.h"

class AAIController;
class APawn;
class UOSESense_Prediction;

USTRUCT()
struct OSEPERCEPTION_API FOSEPredictionEvent
{	
	GENERATED_USTRUCT_BODY()

	typedef UOSESense_Prediction FSenseClass;
	
	UPROPERTY()
	TObjectPtr<AActor> Requestor;

	UPROPERTY()
	TObjectPtr<AActor> PredictedActor;

	float TimeToPredict;
		
	FOSEPredictionEvent() : Requestor(nullptr), PredictedActor(nullptr) {}
	
	FOSEPredictionEvent(AActor* InRequestor, AActor* InPredictedActor, float PredictionTime)
		: Requestor(InRequestor), PredictedActor(InPredictedActor), TimeToPredict(PredictionTime)
	{
	}
};

UCLASS(ClassGroup=AI)
class OSEPERCEPTION_API UOSESense_Prediction : public UOSESense
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TArray<FOSEPredictionEvent> RegisteredEvents;

public:		
	void RegisterEvent(const FOSEPredictionEvent& Event);	

	/** Asks perception system to supply Requestor with PredictedActor's predicted location in PredictionTime seconds
	 *	Location is being predicted based on PredicterActor's current location and velocity */
	UFUNCTION(BlueprintCallable, Category = "OSE|Perception")
	static void RequestControllerPredictionEvent(AAIController* Requestor, AActor* PredictedActor, float PredictionTime);

	/** Asks perception system to supply Requestor with PredictedActor's predicted location in PredictionTime seconds
	 *	Location is being predicted based on PredicterActor's current location and velocity */
	UFUNCTION(BlueprintCallable, Category = "OSE|Perception")
	static void RequestPawnPredictionEvent(APawn* Requestor, AActor* PredictedActor, float PredictionTime);

protected:
	virtual float Update() override;
};
