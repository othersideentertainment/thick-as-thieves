// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#pragma once

// ose
#include "OSESense.h"
#include "OSEPerceptionTypes.h"
#include "OSESense_Sight.h"

// unreal
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "GenericTeamAgentInterface.h"

#include "OSESense_SightFrustum.generated.h"


class UOSESenseConfig_SightFrustum;

UCLASS(ClassGroup = AI, config = Game)
class OSEPERCEPTION_API UOSESense_SightFrustum : public UOSESense_SightBase
{
   GENERATED_UCLASS_BODY()

public:

   struct FSightProperties 
   {
      float SightRadiusSq;
      float LoseSightRadiusSq;
      float HalfFOVInDegrees;
      float PointOfViewBackwardOffset;
      float NearClip;
      float FarClip;
      float LoseSightFarClip;
      float FrustumPitch;
      float FrustumAspectRatio;
   };

   struct FDigestedSightProperties : public UOSESense_SightBase::FDigestedProperties
   {
      FSightProperties Properties;
      float AutoSuccessRangeSqFromLastSeenLocation;
      float AutoSuccessLOSRangeSq;

      FDigestedSightProperties();
      FDigestedSightProperties(const UOSESenseConfig_SightFrustum& SenseConfig);
   };

   TMap<FOSEPerceptionListenerID, FDigestedSightProperties> DigestedProperties;


   virtual bool IsTargetInSightVolume(FOSEPerceptionListener& Listener, const AActor* ListenerActor, FOSESightTarget& Target, AActor* TargetActor, const FDigestedProperties* PropDigest, bool bLastResult) const override;
   virtual const FDigestedProperties* GetDigestedProperties(FOSEPerceptionListenerID ListenerID) const;
protected:

   
   virtual bool _ShouldAutomaticallySeeTarget(const FDigestedProperties* PropDigest, FOSESightQuery* SightQuery, FOSEPerceptionListener& Listener, AActor* TargetActor, float& OutStimulusStrength) const override;
   virtual bool _ShouldNeverSeeTarget(const FDigestedProperties* propDigest, FOSESightQuery* sightQuery, FOSEPerceptionListener& listener, AActor* targetActor) const override;
   virtual bool _ShouldAutoSucceedLOS(const FDigestedProperties* propDigest, const AActor* ListenerActor, AActor* targetActor) const override;

   virtual void _NewListenerImpl(const FOSEPerceptionListener& NewListener) override;
   virtual void _ListenerUpdateImpl(const FOSEPerceptionListener& UpdatedListener) override;
   virtual void _ListenerRemovedImpl(const FOSEPerceptionListener& RemovedListener) override;
   virtual void OnListenerConfigUpdated(const FOSEPerceptionListener& UpdatedListener) override;

private:
   bool _PerformFrustumCheck(const FSightProperties& perAlertDigestedProps, const AActor* bodyActor, FVector observerLocation, FVector targetLocation, FVector targetExtents, bool wasLastQuerySuccessful) const;
};
