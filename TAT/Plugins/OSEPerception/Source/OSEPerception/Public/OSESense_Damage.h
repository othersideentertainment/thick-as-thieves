// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "OSESense.h"
#include "OSESense_Damage.generated.h"

class IOSEPerceptionListenerInterface;
class UOSESenseEvent;

USTRUCT(BlueprintType)
struct OSEPERCEPTION_API FOSEDamageEvent
{	
	GENERATED_USTRUCT_BODY()

	typedef class UOSESense_Damage FSenseClass;

	/** Damage taken by DamagedActor.
	 *	@Note 0-damage events do not get ignored */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
	float Amount;

	/** Event's "Location", or what will be later treated as the perceived location for this sense.
	 *	If not set, HitLocation will be used, if that is unset too DamagedActor's location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
	FVector Location;

	/** Event's additional spatial information
	 *	@TODO document */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
	FVector HitLocation;
	
	/** Damaged actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
	TObjectPtr<AActor> DamagedActor;

	/** Actor that instigated damage. Can be None */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
	TObjectPtr<AActor> Instigator;

	/** Optional named identifier for the damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
	FName Tag;
	
	FOSEDamageEvent();
	FOSEDamageEvent(AActor* InDamagedActor, AActor* InInstigator, float DamageAmount, const FVector& EventLocation, const FVector& InHitLocation = FAISystem::InvalidLocation, FName InTag = NAME_None);
	void Compile();

	bool IsValid() const
	{
		return DamagedActor != nullptr;
	}

	IOSEPerceptionListenerInterface* GetDamagedActorAsPerceptionListener() const;
};

UCLASS(ClassGroup=AI)
class OSEPERCEPTION_API UOSESense_Damage : public UOSESense
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TArray<FOSEDamageEvent> RegisteredEvents;

public:		
	void RegisterEvent(const FOSEDamageEvent& Event);	
	virtual void RegisterWrappedEvent(UOSESenseEvent& PerceptionEvent) override;

	/** EventLocation will be reported as Instigator's location at the moment of event happening*/
	UFUNCTION(BlueprintCallable, Category = "OSE|Perception", meta = (WorldContext="WorldContextObject", AdvancedDisplay="HitLocation"))
	static void ReportDamageEvent(UObject* WorldContextObject, AActor* DamagedActor, AActor* Instigator, float DamageAmount, FVector EventLocation, FVector HitLocation, FName Tag = NAME_None);

protected:
	virtual float Update() override;
};
