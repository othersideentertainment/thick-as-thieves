// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#include "OSESense_SightPeripheral.h"

// ose
#include "AI/Perception/OSEAIPerceptionHelpers.h"
#include "OSESenseConfig_SightPeripheral.h"
#include "OSESense_SightFrustum.h"
#include "OSEPerceptionComponent.h"
#include "OSESightTargetInterface.h"

// ue4
#include "AIHelpers.h"
#include "CollisionQueryParams.h"
//#include "DrawDebugHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESense_SightPeripheral)

DECLARE_CYCLE_STAT(TEXT("OSE Perception Sense: Sight, Listener Update"), STAT_OSE_AI_Sense_Sight_ListenerUpdate, STATGROUP_AI);





//----------------------------------------------------------------------//
// FDigestedSightProperties
//----------------------------------------------------------------------//
UOSESense_SightPeripheral::FDigestedSightProperties::FDigestedSightProperties(const UOSESenseConfig_SightPeripheral& senseConfig)
{
   SightRadius = (senseConfig.SightRadius + senseConfig.PointOfViewBackwardOffset);
   LoseSightRadius = (senseConfig.LoseSightRadius + senseConfig.PointOfViewBackwardOffset);
   PointOfViewBackwardOffset = senseConfig.PointOfViewBackwardOffset;

   // keep the special value of FAISystem::InvalidRange (-1.f) if it's set.
   AutoSuccessRangeSqFromLastSeenLocation = (senseConfig.AutoSuccessRangeFromLastSeenLocation == FAISystem::InvalidRange) ? FAISystem::InvalidRange : FMath::Square(senseConfig.AutoSuccessRangeFromLastSeenLocation);
   AutoSuccessLOSRangeSq = (senseConfig.AutoSuccessLOSRange == FAISystem::InvalidRange) ? FAISystem::InvalidRange : FMath::Square(senseConfig.AutoSuccessLOSRange);
   MaxRadiusSq = FMath::Square(LoseSightRadius *2.f);
   AffiliationFlags = senseConfig.DetectionByAffiliation.GetAsFlags();
}

UOSESense_SightPeripheral::FDigestedSightProperties::FDigestedSightProperties()
{
   SightRadius = -1.0f;
   LoseSightRadius = -1.0f;
   PointOfViewBackwardOffset = 0.0f;
   AutoSuccessRangeSqFromLastSeenLocation = FAISystem::InvalidRange;
   AutoSuccessLOSRangeSq = FAISystem::InvalidRange;
   AffiliationFlags = -1;
}

//----------------------------------------------------------------------//
// UOSESense_SightPeripheral
//----------------------------------------------------------------------//
UOSESense_SightPeripheral::UOSESense_SightPeripheral(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   if (HasAnyFlags(RF_ClassDefaultObject) == false)
   {
      UOSESenseConfig_SightPeripheral* sightConfigCDO = GetMutableDefault<UOSESenseConfig_SightPeripheral>();
      sightConfigCDO->Implementation = UOSESense_SightPeripheral::StaticClass();
   }
}


const UOSESense_SightBase::FDigestedProperties* UOSESense_SightPeripheral::GetDigestedProperties(FOSEPerceptionListenerID ListenerID) const
{
   const FDigestedSightProperties& PropDigest = DigestedProperties[ListenerID];
   return &PropDigest;
}

bool UOSESense_SightPeripheral::IsTargetInSightVolume(FOSEPerceptionListener& listener, const AActor* listenerActor, FOSESightTarget& target, AActor* targetActor, const FDigestedProperties* propDigest, bool bLastResult) const
{
   const FDigestedSightProperties* propDigestSight = reinterpret_cast<const FDigestedSightProperties*>(propDigest);
   
   FVector bodyLocation, bodyFacing;
   listener.Listener->GetLocationAndDirection(bodyLocation, bodyFacing);
   const FVector rootLocation = bodyLocation - (bodyFacing * propDigestSight->PointOfViewBackwardOffset);

   FVector targetLocation;
   FVector targetExtents;
   const bool onlyCollidingComponents = true;
   targetActor->GetActorBounds(onlyCollidingComponents, targetLocation, targetExtents);
   const FBox targetBounds = FBox::BuildAABB(targetLocation, targetExtents);

   bool bInVolume = false;
   if (!bLastResult)
   {
      const FVector sightCenter = rootLocation + (bodyFacing * propDigestSight->SightRadius);
      const FSphere sightSphere(sightCenter, propDigestSight->SightRadius);

      //DrawDebugBox(targetActor->GetWorld(), targetLocation, targetExtents, FColor::Blue, false, -1.0f, 0U, 2.0f);
      //DrawDebugSphere(targetActor->GetWorld(), sightCenter, PropDigestSight->SightRadius, 16, FColor::Red, false, -1.0f, 0U, 2.0f);

      bInVolume = FMath::SphereAABBIntersection(sightSphere, targetBounds);
   }
   else
   {
      const FVector loseSightCenter = rootLocation + (bodyFacing * propDigestSight->LoseSightRadius);
      const FSphere loseSightSphere(loseSightCenter, propDigestSight->LoseSightRadius);

      //DrawDebugBox(targetActor->GetWorld(), targetLocation, targetExtents, FColor::Blue, false, -1.0f, 0U, 2.0f);
      //DrawDebugSphere(targetActor->GetWorld(), loseSightCenter, PropDigestSight->LoseSightRadius, 16, FColor::Red, false, -1.0f, 0U, 2.0f);

      bInVolume = FMath::SphereAABBIntersection(loseSightSphere, targetBounds);
   }

   if (bInVolume)
   {
      const FOSESenseID sightSenseID = UOSESense::GetSenseID<UOSESense_SightFrustum>();
      if (listener.HasSense(sightSenseID))
      {
         UOSEPerceptionSystem* perceptionSys = GetPerceptionSystem();
         UOSESense* sense = perceptionSys->GetSense(sightSenseID);
         UOSESense_SightFrustum* frustum = Cast<UOSESense_SightFrustum>(sense);
         const FDigestedProperties* frustumDigest = frustum->GetDigestedProperties(listener.GetListenerID());

         bInVolume &= !frustum->IsTargetInSightVolume(listener, listenerActor, target, targetActor, frustumDigest, bLastResult);
      }
   }


   return bInVolume;

}

bool UOSESense_SightPeripheral::_ShouldAutomaticallySeeTarget(const FDigestedProperties* propDigest, FOSESightQuery* sightQuery, FOSEPerceptionListener& listener, AActor* targetActor, float& outStimulusStrength) const
{
   const FDigestedSightProperties* propDigestSight = reinterpret_cast<const FDigestedSightProperties*>(propDigest);
   if ((propDigestSight->AutoSuccessRangeSqFromLastSeenLocation != FAISystem::InvalidRange) && (sightQuery->LastSeenLocation != FAISystem::InvalidLocation))
   {
      const float distanceToLastSeenLocationSq = FVector::DistSquared(targetActor->GetActorLocation(), sightQuery->LastSeenLocation);
      return (distanceToLastSeenLocationSq <= propDigestSight->AutoSuccessRangeSqFromLastSeenLocation);
   }

   return false;
}

bool UOSESense_SightPeripheral::_ShouldNeverSeeTarget(const FDigestedProperties* propDigest, FOSESightQuery* sightQuery, FOSEPerceptionListener& listener, AActor* targetActor) const
{
   return false;
}

bool UOSESense_SightPeripheral::_ShouldAutoSucceedLOS(const FDigestedProperties* propDigest, const AActor* ListenerActor, AActor* targetActor) const
{
   const FDigestedSightProperties* propDigestSight = reinterpret_cast<const FDigestedSightProperties*>(propDigest);
   if ((propDigestSight->AutoSuccessLOSRangeSq != FAISystem::InvalidRange))
   {
      const float distanceToLastSeenLocationSq = FVector::DistSquared(targetActor->GetActorLocation(), ListenerActor->GetActorLocation());
      return (distanceToLastSeenLocationSq <= propDigestSight->AutoSuccessLOSRangeSq);
   }
   return false;
}


void UOSESense_SightPeripheral::_NewListenerImpl(const FOSEPerceptionListener& NewListener)
{
   UOSEPerceptionComponent* NewListenerPtr = NewListener.Listener.Get();
   check(NewListenerPtr);
   const UOSESenseConfig_SightPeripheral* SenseConfig = Cast<const UOSESenseConfig_SightPeripheral>(NewListenerPtr->GetSenseConfig(GetSenseID()));
   check(SenseConfig);
   const FDigestedSightProperties PropertyDigest(*SenseConfig);
   DigestedProperties.Add(NewListener.GetListenerID(), PropertyDigest);

   Super::_NewListenerImpl(NewListener);
}


void UOSESense_SightPeripheral::_ListenerUpdateImpl(const FOSEPerceptionListener& UpdatedListener)
{
   SCOPE_CYCLE_COUNTER(STAT_OSE_AI_Sense_Sight_ListenerUpdate);

   const FOSEPerceptionListenerID ListenerID = UpdatedListener.GetListenerID();

   if (UpdatedListener.HasSense(GetSenseID()))
   {
      const UOSESenseConfig_SightPeripheral* SenseConfig = Cast<const UOSESenseConfig_SightPeripheral>(UpdatedListener.Listener->GetSenseConfig(GetSenseID()));
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

void UOSESense_SightPeripheral::OnListenerConfigUpdated(const FOSEPerceptionListener& UpdatedListener)
{
   bool bSkipListenerUpdate = false;
   const FOSEPerceptionListenerID ListenerID = UpdatedListener.GetListenerID();

   FDigestedSightProperties* PropertiesDigest = DigestedProperties.Find(ListenerID);
   if (PropertiesDigest)
   {
      // The only parameter we need to rebuild all the queries for this listener is if the affiliation mask changed, otherwise there is nothing to update.
      const UOSESenseConfig_SightPeripheral* SenseConfig = CastChecked<const UOSESenseConfig_SightPeripheral>(UpdatedListener.Listener->GetSenseConfig(GetSenseID()));
      FDigestedSightProperties NewPropertiesDigest(*SenseConfig);
      bSkipListenerUpdate = NewPropertiesDigest.AffiliationFlags == PropertiesDigest->AffiliationFlags;
      *PropertiesDigest = NewPropertiesDigest;
   }

   if (!bSkipListenerUpdate)
   {
      Super::OnListenerConfigUpdated(UpdatedListener);
   }
}


void UOSESense_SightPeripheral::_ListenerRemovedImpl(const FOSEPerceptionListener& RemovedListener)
{
   Super::_ListenerRemovedImpl(RemovedListener);

   DigestedProperties.FindAndRemoveChecked(RemovedListener.GetListenerID());
}
