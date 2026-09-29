// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#pragma once

//OSE
#include "OSESense.h"
#include "OSESense_SightBase.h"

//UE
#include "CoreMinimal.h"


#include "OSESense_Sight.generated.h"

class UOSESenseConfig_Sight;

UCLASS(ClassGroup=AI, config=Game)
class OSEPERCEPTION_API UOSESense_Sight : public UOSESense_SightBase
{
	GENERATED_UCLASS_BODY()

public:
	struct FDigestedSightProperties : public UOSESense_SightBase::FDigestedProperties
	{
		float PeripheralVisionAngleCos;
		float SightRadiusSq;
		float AutoSuccessRangeSqFromLastSeenLocation;
		float LoseSightRadiusSq;
		float PointOfViewBackwardOffset;
		float NearClippingRadiusSq;

		FDigestedSightProperties();
		FDigestedSightProperties(const UOSESenseConfig_Sight& SenseConfig);
	};	

   virtual bool IsTargetInSightVolume(FOSEPerceptionListener& Listener, const AActor* ListenerActor, FOSESightTarget& Target, AActor* TargetActor, const FDigestedProperties* PropDigest, bool bLastResult) const override;
   virtual const FDigestedProperties* GetDigestedProperties(FOSEPerceptionListenerID ListenerID) const;

protected:
	TMap<FOSEPerceptionListenerID, FDigestedSightProperties> DigestedProperties;

protected:
   
   
   virtual bool _ShouldAutomaticallySeeTarget(const FDigestedProperties* PropDigest, FOSESightQuery* SightQuery, FOSEPerceptionListener& Listener, AActor* TargetActor, float& OutStimulusStrength) const override;
   virtual bool _ShouldNeverSeeTarget(const FDigestedProperties* propDigest, FOSESightQuery* sightQuery, FOSEPerceptionListener& listener, AActor* targetActor) const override;


   virtual void _NewListenerImpl(const FOSEPerceptionListener& NewListener) override;
   virtual void _ListenerUpdateImpl(const FOSEPerceptionListener& UpdatedListener) override;
   virtual void _ListenerRemovedImpl(const FOSEPerceptionListener& RemovedListener) override;
	virtual void OnListenerConfigUpdated(const FOSEPerceptionListener& UpdatedListener) override;

};
