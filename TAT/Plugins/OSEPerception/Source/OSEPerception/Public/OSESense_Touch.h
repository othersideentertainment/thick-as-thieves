// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "OSESense.h"
#include "OSESense_Touch.generated.h"

class IOSEPerceptionListenerInterface;
class UOSESense_Touch;

USTRUCT()
struct OSEPERCEPTION_API FOSETouchEvent
{	
	GENERATED_USTRUCT_BODY()

	typedef UOSESense_Touch FSenseClass;

	FVector Location;
	
	UPROPERTY()
	TObjectPtr<AActor> TouchReceiver;

	UPROPERTY()
	TObjectPtr<AActor> OtherActor;
		
	FOSETouchEvent() : TouchReceiver(nullptr), OtherActor(nullptr) {}
	
	FOSETouchEvent(AActor* InTouchReceiver, AActor* InOtherActor, const FVector& EventLocation)
		: Location(EventLocation), TouchReceiver(InTouchReceiver), OtherActor(InOtherActor)
	{
	}

	IOSEPerceptionListenerInterface* GetTouchedActorAsPerceptionListener() const;
};

UCLASS(ClassGroup=AI)
class OSEPERCEPTION_API UOSESense_Touch : public UOSESense
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TArray<FOSETouchEvent> RegisteredEvents;

public:		
	void RegisterEvent(const FOSETouchEvent& Event);	

	UFUNCTION(BlueprintCallable, Category = "OSE|Perception", meta = (WorldContext = "WorldContextObject"))
	static void ReportTouchEvent(UObject* WorldContextObject, AActor* TouchReceiver, AActor* OtherActor, FVector Location);

protected:
	virtual float Update() override;
};
