// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "GenericTeamAgentInterface.h"
#include "OSESense.h"
#include "OSESense_Team.generated.h"

class UOSESense_Team;

USTRUCT()
struct OSEPERCEPTION_API FOSETeamStimulusEvent
{	
	GENERATED_USTRUCT_BODY()

	typedef UOSESense_Team FSenseClass;

	FVector LastKnowLocation;
private:
	FVector BroadcastLocation;
public:
	float RangeSq;
	float InformationAge;
	FGenericTeamId TeamIdentifier;
	float Strength;
private:
	UPROPERTY()
	TObjectPtr<AActor> Broadcaster;
public:
	UPROPERTY()
	TObjectPtr<AActor> Enemy;
		
	FOSETeamStimulusEvent() : Broadcaster(nullptr), Enemy(nullptr) {}
	FOSETeamStimulusEvent(AActor* InBroadcaster, AActor* InEnemy, const FVector& InLastKnowLocation, float EventRange, float PassedInfoAge = 0.f, float InStrength = 1.f);

	FORCEINLINE void CacheBroadcastLocation()
	{
		BroadcastLocation = Broadcaster ? Broadcaster->GetActorLocation() : FAISystem::InvalidLocation;
	}

	FORCEINLINE const FVector& GetBroadcastLocation() const 
	{
		return BroadcastLocation;
	}
};

UCLASS(ClassGroup=AI)
class OSEPERCEPTION_API UOSESense_Team : public UOSESense
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TArray<FOSETeamStimulusEvent> RegisteredEvents;

public:		
	void RegisterEvent(const FOSETeamStimulusEvent& Event);	

protected:
	virtual float Update() override;
};
