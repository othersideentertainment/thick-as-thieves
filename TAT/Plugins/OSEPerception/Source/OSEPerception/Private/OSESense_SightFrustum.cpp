// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#include "OSESense_SightFrustum.h"

// ose
#include "AI/Perception/OSEAIPerceptionHelpers.h"
#include "OSESenseConfig_SightFrustum.h"
#include "OSEPerceptionComponent.h"
#include "OSESightTargetInterface.h"

// ue4
#include "AIHelpers.h"
#include "CollisionQueryParams.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESense_SightFrustum)

DECLARE_CYCLE_STAT(TEXT("OSE Perception Sense: Sight, Listener Update"), STAT_OSE_AI_Sense_Sight_ListenerUpdate, STATGROUP_AI);





//----------------------------------------------------------------------//
// FDigestedSightProperties
//----------------------------------------------------------------------//
UOSESense_SightFrustum::FDigestedSightProperties::FDigestedSightProperties(const UOSESenseConfig_SightFrustum& senseConfig)
{
   Properties.SightRadiusSq = FMath::Square(senseConfig.FrustumSettings.SightRadius + senseConfig.FrustumSettings.PointOfViewBackwardOffset);
   Properties.LoseSightRadiusSq = FMath::Square(senseConfig.FrustumSettings.LoseSightRadius + senseConfig.FrustumSettings.PointOfViewBackwardOffset);
   Properties.HalfFOVInDegrees = senseConfig.FrustumSettings.VisionHorizontalAngleDegrees;
   Properties.PointOfViewBackwardOffset = senseConfig.FrustumSettings.PointOfViewBackwardOffset;
   Properties.NearClip = senseConfig.FrustumSettings.NearClippingRadius;
   Properties.FarClip = senseConfig.FrustumSettings.SightRadius;
   Properties.LoseSightFarClip = senseConfig.FrustumSettings.LoseSightRadius;
   Properties.FrustumPitch = senseConfig.FrustumSettings.FrustumPitch;
   Properties.FrustumAspectRatio = senseConfig.FrustumSettings.VisionHorizontalAngleDegrees / senseConfig.FrustumSettings.VisionVerticalAngleDegrees;

   if (senseConfig.AutoSuccessRangeFromLastSeenLocation == FAISystem::InvalidRange)
   {
      AutoSuccessRangeSqFromLastSeenLocation = FAISystem::InvalidRange;
   }
   else
   {
      AutoSuccessRangeSqFromLastSeenLocation = FMath::Square(senseConfig.AutoSuccessRangeFromLastSeenLocation);
   }

   AutoSuccessLOSRangeSq = (senseConfig.AutoSuccessLOSRange == FAISystem::InvalidRange) ? FAISystem::InvalidRange : FMath::Square(senseConfig.AutoSuccessLOSRange);
   MaxRadiusSq = Properties.LoseSightRadiusSq;
   AffiliationFlags = senseConfig.DetectionByAffiliation.GetAsFlags();
}

UOSESense_SightFrustum::FDigestedSightProperties::FDigestedSightProperties()
{
   Properties.SightRadiusSq = -1.0f;
   Properties.LoseSightRadiusSq = -1.0f;
   Properties.HalfFOVInDegrees = 0.0f;
   Properties.PointOfViewBackwardOffset = 0.0f;
   Properties.NearClip = 0.0f;
   Properties.FarClip = 0.0f;
   Properties.LoseSightFarClip = 0.0f;
   Properties.FrustumPitch = -18.0f;
   Properties.FrustumAspectRatio = 2.2f;
   AutoSuccessRangeSqFromLastSeenLocation = FAISystem::InvalidRange;
   AutoSuccessLOSRangeSq = FAISystem::InvalidRange;
   AffiliationFlags = -1;
}

//----------------------------------------------------------------------//
// UOSESense_SightFrustum
//----------------------------------------------------------------------//
UOSESense_SightFrustum::UOSESense_SightFrustum(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   if (HasAnyFlags(RF_ClassDefaultObject) == false)
   {
      UOSESenseConfig_SightFrustum* sightConfigCDO = GetMutableDefault<UOSESenseConfig_SightFrustum>();
      sightConfigCDO->Implementation = UOSESense_SightFrustum::StaticClass();
   }
}

bool UOSESense_SightFrustum::_PerformFrustumCheck(const FSightProperties& perAlertDigestedProps, const AActor* bodyActor, FVector observerLocation, FVector targetLocation, FVector targetExtents, bool wasLastQuerySuccessful) const
{
   // If the pawn that this controller is attached to is possessed by another controller (e.g. a player),
   // then our bodyActor will be null. In that case, we should skip any frustum checks since we don't know
   // where the pawn is anymore. We could use the cached location/direction, but this will be wrong as the pawn moves
   // away from where it was the last time we controlled it
   if (!bodyActor)
   {
      return false;
   }

   const float halfFOVInDegrees = perAlertDigestedProps.HalfFOVInDegrees;
   const float backwardOffset = perAlertDigestedProps.PointOfViewBackwardOffset;
   const float nearClip = perAlertDigestedProps.NearClip;
   const float farClip = wasLastQuerySuccessful ? perAlertDigestedProps.LoseSightFarClip : perAlertDigestedProps.FarClip;

   FVector ignoredLocation;
   FRotator frustumRotation;
   bodyActor->GetActorEyesViewPoint(ignoredLocation, frustumRotation);

   // Sight cone offsets backwards based on view direction
   const FVector frustumOrigin = observerLocation - (frustumRotation.Vector() * backwardOffset);

   return OSEAIPerceptionHelpers::CheckIsTargetInFrustum(
      bodyActor->GetWorld(),
      frustumOrigin,
      frustumRotation,
      targetLocation,
      targetExtents,
      halfFOVInDegrees,
      nearClip,
      farClip,
      perAlertDigestedProps.FrustumPitch,
      perAlertDigestedProps.FrustumAspectRatio);
}

const UOSESense_SightBase::FDigestedProperties* UOSESense_SightFrustum::GetDigestedProperties(FOSEPerceptionListenerID ListenerID) const
{
   const FDigestedSightProperties& PropDigest = DigestedProperties[ListenerID];
   return &PropDigest;
}

bool UOSESense_SightFrustum::IsTargetInSightVolume(FOSEPerceptionListener& listener, const AActor* listenerActor, FOSESightTarget& target, AActor* targetActor, const FDigestedProperties* propDigest, bool bLastResult) const
{
   const FDigestedSightProperties* propDigestSight = reinterpret_cast<const FDigestedSightProperties*>(propDigest);
   ensure(propDigest->MaxRadiusSq == propDigestSight->MaxRadiusSq);
   FVector targetLocation;
   FVector targetExtents;
   const bool onlyCollidingComponents = true;
   targetActor->GetActorBounds(onlyCollidingComponents, targetLocation, targetExtents);
   return _PerformFrustumCheck(propDigestSight->Properties, listener.GetBodyActor(), listener.CachedLocation, targetLocation, targetExtents, bLastResult);
}

bool UOSESense_SightFrustum::_ShouldAutomaticallySeeTarget(const FDigestedProperties* propDigest, FOSESightQuery* sightQuery, FOSEPerceptionListener& listener, AActor* targetActor, float& outStimulusStrength) const
{
   const FDigestedSightProperties* propDigestSight = reinterpret_cast<const FDigestedSightProperties*>(propDigest);
   ensure(propDigest->MaxRadiusSq == propDigestSight->MaxRadiusSq);
   if ((propDigestSight->AutoSuccessRangeSqFromLastSeenLocation != FAISystem::InvalidRange) && (sightQuery->LastSeenLocation != FAISystem::InvalidLocation))
   {
      const float distanceToLastSeenLocationSq = FVector::DistSquared(targetActor->GetActorLocation(), sightQuery->LastSeenLocation);
      return (distanceToLastSeenLocationSq <= propDigestSight->AutoSuccessRangeSqFromLastSeenLocation);
   }

   return false;
}

bool UOSESense_SightFrustum::_ShouldNeverSeeTarget(const FDigestedProperties* propDigest, FOSESightQuery* sightQuery, FOSEPerceptionListener& listener, AActor* targetActor) const
{
   //if (const IGameplayTagAssetInterface* tagInterface = Cast<IGameplayTagAssetInterface>(targetActor))
   //{
   //   const UOSEAISettings& settings = UOSEAISettings::Get();
   //   if (tagInterface->HasAnyMatchingGameplayTags(settings.DoNotSeeActorTags))
   //      return true;
   //}

   //if (const IOSEAISightInterface* sightInterface = Cast<IOSEAISightInterface>(listener.GetBodyActor()))
   //{
   //   if (!sightInterface->IsAllowedToSeeActor(targetActor))
   //      return true;
   //}

   return false;
}


bool UOSESense_SightFrustum::_ShouldAutoSucceedLOS(const FDigestedProperties* propDigest, const AActor* ListenerActor, AActor* targetActor) const
{
   const FDigestedSightProperties* propDigestSight = reinterpret_cast<const FDigestedSightProperties*>(propDigest);
   if ((propDigestSight->AutoSuccessLOSRangeSq != FAISystem::InvalidRange))
   {
      const float distanceToLastSeenLocationSq = FVector::DistSquared(targetActor->GetActorLocation(), ListenerActor->GetActorLocation());
      return (distanceToLastSeenLocationSq <= propDigestSight->AutoSuccessLOSRangeSq);
   }
   return false;
}

void UOSESense_SightFrustum::_NewListenerImpl(const FOSEPerceptionListener& NewListener)
{
   UOSEPerceptionComponent* NewListenerPtr = NewListener.Listener.Get();
   check(NewListenerPtr);
   const UOSESenseConfig_SightFrustum* SenseConfig = Cast<const UOSESenseConfig_SightFrustum>(NewListenerPtr->GetSenseConfig(GetSenseID()));
   check(SenseConfig);
   const FDigestedSightProperties PropertyDigest(*SenseConfig);
   DigestedProperties.Add(NewListener.GetListenerID(), PropertyDigest);

   Super::_NewListenerImpl(NewListener);
}


void UOSESense_SightFrustum::_ListenerUpdateImpl(const FOSEPerceptionListener& UpdatedListener)
{
   SCOPE_CYCLE_COUNTER(STAT_OSE_AI_Sense_Sight_ListenerUpdate);

   const FOSEPerceptionListenerID ListenerID = UpdatedListener.GetListenerID();

   if (UpdatedListener.HasSense(GetSenseID()))
   {
      const UOSESenseConfig_SightFrustum* SenseConfig = Cast<const UOSESenseConfig_SightFrustum>(UpdatedListener.Listener->GetSenseConfig(GetSenseID()));
      check(SenseConfig);
      FDigestedSightProperties& PropertiesDigest = DigestedProperties.FindOrAdd(ListenerID);
      PropertiesDigest = FDigestedSightProperties(*SenseConfig);
   }
   else
   {
      DigestedProperties.Remove(ListenerID);
   }

   Super::_ListenerUpdateImpl(UpdatedListener);
}

void UOSESense_SightFrustum::OnListenerConfigUpdated(const FOSEPerceptionListener& UpdatedListener)
{
   bool bSkipListenerUpdate = false;
   const FOSEPerceptionListenerID ListenerID = UpdatedListener.GetListenerID();

   FDigestedSightProperties* PropertiesDigest = DigestedProperties.Find(ListenerID);
   if (PropertiesDigest)
   {
      // The only parameter we need to rebuild all the queries for this listener is if the affiliation mask changed, otherwise there is nothing to update.
      const UOSESenseConfig_SightFrustum* SenseConfig = CastChecked<const UOSESenseConfig_SightFrustum>(UpdatedListener.Listener->GetSenseConfig(GetSenseID()));
      FDigestedSightProperties NewPropertiesDigest(*SenseConfig);
      bSkipListenerUpdate = NewPropertiesDigest.AffiliationFlags == PropertiesDigest->AffiliationFlags;
      *PropertiesDigest = NewPropertiesDigest;
   }

   if (!bSkipListenerUpdate)
   {
      Super::OnListenerConfigUpdated(UpdatedListener);
   }
}


void UOSESense_SightFrustum::_ListenerRemovedImpl(const FOSEPerceptionListener& RemovedListener)
{
   Super::_ListenerRemovedImpl(RemovedListener);

   DigestedProperties.FindAndRemoveChecked(RemovedListener.GetListenerID());
}
