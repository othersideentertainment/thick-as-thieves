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

#include "OSESense_SightPeripheral.generated.h"


class UOSESenseConfig_SightPeripheral;

UCLASS(ClassGroup = AI, config = Game)
class OSEPERCEPTION_API UOSESense_SightPeripheral : public UOSESense_SightBase
{
   GENERATED_UCLASS_BODY()

public:

   struct FDigestedSightProperties : public UOSESense_SightBase::FDigestedProperties
   {
      float SightRadius;
      float LoseSightRadius;
      float PointOfViewBackwardOffset;

      float AutoSuccessRangeSqFromLastSeenLocation;
      float AutoSuccessLOSRangeSq;

      FDigestedSightProperties();
      FDigestedSightProperties(const UOSESenseConfig_SightPeripheral& SenseConfig);
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
};
