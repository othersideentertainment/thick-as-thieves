// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "GenericTeamAgentInterface.h"
#include "OSESense.h"
#include "OSESense_Hearing.generated.h"

class UOSESenseConfig_Hearing;
class UOSESenseEvent;

USTRUCT(BlueprintType)
struct OSEPERCEPTION_API FOSENoiseEvent
{	
	GENERATED_USTRUCT_BODY()

	typedef class UOSESense_Hearing FSenseClass;

	float Age;

	/** if not set Instigator's location will be used */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
	FVector NoiseLocation;

	/**
	 * Loudness modifier of the sound.
	 * If MaxRange is non-zero, this modifies the range (by multiplication).
	 * If there is no MaxRange, then if Square(DistanceToSound) <= Square(HearingRange) * Loudness, the sound is heard, false otherwise.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense", meta = (UIMin = 0, ClampMin = 0))
	float Loudness;

	/**
	 * Max range at which the sound can be heard. Multiplied by Loudness.
	 * A value of 0 indicates that there is no range limit, though listeners are still limited by their own hearing range.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense", meta = (UIMin = 0, ClampMin = 0))
	float MaxRange;
	
	/**
	 * Actor triggering the sound.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
	TObjectPtr<AActor> Instigator;

	/**
	 * Named identifier for the noise.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
	FName Tag;

	FGenericTeamId TeamIdentifier;
		
	FOSENoiseEvent();
	FOSENoiseEvent(AActor* InInstigator, const FVector& InNoiseLocation, float InLoudness = 1.f, float InMaxRange = 0.f, FName Tag = NAME_None);

	/** Verifies and calculates derived data */
	void Compile();
};

UCLASS(ClassGroup=AI, Config=Game)
class OSEPERCEPTION_API UOSESense_Hearing : public UOSESense
{
	GENERATED_UCLASS_BODY()
		
protected:
	UPROPERTY()
	TArray<FOSENoiseEvent> NoiseEvents;

	/** Defaults to 0 to have instant notification. Setting to > 0 will result in delaying 
	 *	when AI hears the sound based on the distance from the source */
	UPROPERTY(config)
	float SpeedOfSoundSq;

	struct FDigestedHearingProperties
	{
		float HearingRangeSq;
		float LoSHearingRangeSq;
		uint8 AffiliationFlags;
		uint32 bUseLoSHearing : 1;

		FDigestedHearingProperties(const UOSESenseConfig_Hearing& SenseConfig);
		FDigestedHearingProperties();
	};
	TMap<FOSEPerceptionListenerID, FDigestedHearingProperties> DigestedProperties;

public:	
	void RegisterEvent(const FOSENoiseEvent& Event);	
	void RegisterEventsBatch(const TArray<FOSENoiseEvent>& Events);

	virtual void PostInitProperties() override;

	// part of BP interface. Translates PerceptionEvent to FOSENoiseEvent and call RegisterEvent(const FOSENoiseEvent& Event)
	virtual void RegisterWrappedEvent(UOSESenseEvent& PerceptionEvent) override;

	/**
	 * Report a noise event.
	 * 
	 * @param NoiseLocation Location of the noise.
	 * @param Loudness Loudness of the noise. If MaxRange is non-zero, modifies MaxRange, otherwise modifies the squared distance of the sensor's range.
	 * @param Instigator Actor that triggered the noise.
	 * @param MaxRange Max range at which the sound can be heard, multiplied by Loudness. Values <= 0 mean no limit (still limited by listener's range however).
	 * @param Tag Identifier for the event.
	 */
	UFUNCTION(BlueprintCallable, Category = "OSE|Perception", meta = (WorldContext="WorldContextObject"))
	static void ReportNoiseEvent(UObject* WorldContextObject, FVector NoiseLocation, float Loudness = 1.f, AActor* Instigator = nullptr, float MaxRange = 0.f, FName Tag = NAME_None);

protected:
	virtual float Update() override;
	virtual void RegisterMakeNoiseDelegate();

	void OnNewListenerImpl(const FOSEPerceptionListener& NewListener);
	void OnListenerUpdateImpl(const FOSEPerceptionListener& UpdatedListener);
	void OnListenerRemovedImpl(const FOSEPerceptionListener& UpdatedListener);
};
