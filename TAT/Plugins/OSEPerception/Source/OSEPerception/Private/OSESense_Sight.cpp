// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

//OSE
#include "OSESense_Sight.h"
#include "OSEPerceptionComponent.h"
#include "OSESenseConfig_Sight.h"

//UE
#include "AIHelpers.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESense_Sight)

DECLARE_CYCLE_STAT(TEXT("Perception Sense: Sight, Listener Update"), STAT_AI_Sense_Sight_ListenerUpdate, STATGROUP_AI);


//----------------------------------------------------------------------//
// FDigestedSightProperties
//----------------------------------------------------------------------//
UOSESense_Sight::FDigestedSightProperties::FDigestedSightProperties(const UOSESenseConfig_Sight& SenseConfig)
{
	SightRadiusSq = FMath::Square(SenseConfig.SightRadius + SenseConfig.PointOfViewBackwardOffset);
	LoseSightRadiusSq = FMath::Square(SenseConfig.LoseSightRadius + SenseConfig.PointOfViewBackwardOffset);
   MaxRadiusSq = LoseSightRadiusSq;
	PointOfViewBackwardOffset = SenseConfig.PointOfViewBackwardOffset;
	NearClippingRadiusSq = FMath::Square(SenseConfig.NearClippingRadius);
	PeripheralVisionAngleCos = FMath::Cos(FMath::Clamp(FMath::DegreesToRadians(SenseConfig.PeripheralVisionAngleDegrees), 0.f, PI));
	AffiliationFlags = SenseConfig.DetectionByAffiliation.GetAsFlags();
	// keep the special value of FAISystem::InvalidRange (-1.f) if it's set.
	AutoSuccessRangeSqFromLastSeenLocation = (SenseConfig.AutoSuccessRangeFromLastSeenLocation == FAISystem::InvalidRange) ? FAISystem::InvalidRange : FMath::Square(SenseConfig.AutoSuccessRangeFromLastSeenLocation);
}

UOSESense_Sight::FDigestedSightProperties::FDigestedSightProperties()
	: PeripheralVisionAngleCos(0.f), SightRadiusSq(-1.f), AutoSuccessRangeSqFromLastSeenLocation(FAISystem::InvalidRange), LoseSightRadiusSq(-1.f), PointOfViewBackwardOffset(0.0f), NearClippingRadiusSq(0.0f)
{}

//----------------------------------------------------------------------//
// UOSESense_Sight
//----------------------------------------------------------------------//
UOSESense_Sight::UOSESense_Sight(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		UOSESenseConfig_Sight* SightConfigCDO = GetMutableDefault<UOSESenseConfig_Sight>();
		SightConfigCDO->Implementation = UOSESense_Sight::StaticClass();
	}
}

const UOSESense_SightBase::FDigestedProperties* UOSESense_Sight::GetDigestedProperties(FOSEPerceptionListenerID ListenerID) const
{
   const FDigestedSightProperties& PropDigest = DigestedProperties[ListenerID];
   return &PropDigest;
}

bool UOSESense_Sight::IsTargetInSightVolume(FOSEPerceptionListener& Listener, const AActor* ListenerActor, FOSESightTarget& Target, AActor* TargetActor, const FDigestedProperties* PropDigest, bool bLastResult) const
{
   const FDigestedSightProperties* PropDigestSight = reinterpret_cast<const FDigestedSightProperties*>(PropDigest);
   const FVector TargetLocation = TargetActor->GetActorLocation();
   const float SightRadiusSq = bLastResult ? PropDigestSight->LoseSightRadiusSq : PropDigestSight->SightRadiusSq;
   if (!FAISystem::CheckIsTargetInSightCone(Listener.CachedLocation, Listener.CachedDirection, PropDigestSight->PeripheralVisionAngleCos, PropDigestSight->PointOfViewBackwardOffset, PropDigestSight->NearClippingRadiusSq, SightRadiusSq, TargetLocation))
   {
      return false;
   }
   return true;
}

bool UOSESense_Sight::_ShouldAutomaticallySeeTarget(const FDigestedProperties* PropDigest, FOSESightQuery* SightQuery, FOSEPerceptionListener& Listener, AActor* TargetActor, float& OutStimulusStrength) const
{
	OutStimulusStrength = 1.0f;
   
   const FDigestedSightProperties* PropDigestSight = reinterpret_cast<const FDigestedSightProperties*>(PropDigest);
	if ((PropDigestSight->AutoSuccessRangeSqFromLastSeenLocation != FAISystem::InvalidRange) && (SightQuery->LastSeenLocation != FAISystem::InvalidLocation))
	{
		const float DistanceToLastSeenLocationSq = FVector::DistSquared(TargetActor->GetActorLocation(), SightQuery->LastSeenLocation);
		return (DistanceToLastSeenLocationSq <= PropDigestSight->AutoSuccessRangeSqFromLastSeenLocation);
	}

	return false;
}

bool UOSESense_Sight::_ShouldNeverSeeTarget(const FDigestedProperties* propDigest, FOSESightQuery* sightQuery, FOSEPerceptionListener& listener, AActor* targetActor) const
{
   return false;
}

void UOSESense_Sight::_NewListenerImpl(const FOSEPerceptionListener& NewListener)
{
	UOSEPerceptionComponent* NewListenerPtr = NewListener.Listener.Get();
	check(NewListenerPtr);
	const UOSESenseConfig_Sight* SenseConfig = Cast<const UOSESenseConfig_Sight>(NewListenerPtr->GetSenseConfig(GetSenseID()));
	check(SenseConfig);
	const FDigestedSightProperties PropertyDigest(*SenseConfig);
	DigestedProperties.Add(NewListener.GetListenerID(), PropertyDigest);

   //This needs to go here, as the Properties added above will be used
   Super::_NewListenerImpl(NewListener);
}


void UOSESense_Sight::_ListenerUpdateImpl(const FOSEPerceptionListener& UpdatedListener)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_Sense_Sight_ListenerUpdate);

	const FOSEPerceptionListenerID ListenerID = UpdatedListener.GetListenerID();

	if (UpdatedListener.HasSense(GetSenseID()))
	{
		const UOSESenseConfig_Sight* SenseConfig = Cast<const UOSESenseConfig_Sight>(UpdatedListener.Listener->GetSenseConfig(GetSenseID()));
		check(SenseConfig);
		FDigestedSightProperties& PropertiesDigest = DigestedProperties.FindOrAdd(ListenerID);
		PropertiesDigest = FDigestedSightProperties(*SenseConfig);
	}
	else
	{
		DigestedProperties.Remove(ListenerID);
	}

   //This needs to go here, as the Properties updated above will be used
   Super::_ListenerUpdateImpl(UpdatedListener);
}

void UOSESense_Sight::_ListenerRemovedImpl(const FOSEPerceptionListener& RemovedListener)
{
   Super::_ListenerRemovedImpl(RemovedListener);

   DigestedProperties.FindAndRemoveChecked(RemovedListener.GetListenerID());
}

void UOSESense_Sight::OnListenerConfigUpdated(const FOSEPerceptionListener& UpdatedListener)
{
	bool bSkipListenerUpdate = false;
	const FOSEPerceptionListenerID ListenerID = UpdatedListener.GetListenerID();

	FDigestedSightProperties* PropertiesDigest = DigestedProperties.Find(ListenerID);
	if (PropertiesDigest)
	{
		// The only parameter we need to rebuild all the queries for this listener is if the affiliation mask changed, otherwise there is nothing to update.
		const UOSESenseConfig_Sight* SenseConfig = CastChecked<const UOSESenseConfig_Sight>(UpdatedListener.Listener->GetSenseConfig(GetSenseID()));
		FDigestedSightProperties NewPropertiesDigest(*SenseConfig);
		bSkipListenerUpdate = NewPropertiesDigest.AffiliationFlags == PropertiesDigest->AffiliationFlags;
		*PropertiesDigest = NewPropertiesDigest;
	}

	if (!bSkipListenerUpdate)
	{
		Super::OnListenerConfigUpdated(UpdatedListener);
	}
}




