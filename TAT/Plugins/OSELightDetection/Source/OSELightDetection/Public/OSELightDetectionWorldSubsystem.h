// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "OSELightDetectionTypes.h"

// ue
#include "OSELightDetectionInterface.h"
#include "Subsystems/WorldSubsystem.h"

#include "OSELightDetectionWorldSubsystem.generated.h"

class UOSELightDetectionSettings;
class UOSELightEmittingComponent;
class UOSELightDetectionComponent;

UCLASS()
class OSELIGHTDETECTION_API UOSELightDetectionWorldSubsystem : public UTickableWorldSubsystem
{
   GENERATED_BODY()
public:
   virtual void Initialize(FSubsystemCollectionBase& collection) override;
   virtual void OnWorldBeginPlay(UWorld& inWorld) override;
   virtual void OnWorldComponentsUpdated(UWorld& world) override;
   void RegisterLight(UOSELightEmittingComponent* lightEmittingComponent);
   void UnRegisterLight(UOSELightEmittingComponent* lightEmittingComponent);

   void RegisterLightDetectionComponent(UOSELightDetectionComponent* lightDetectionComponent);
   void UnRegisterLightDetectionComponent(UOSELightDetectionComponent* lightDetectionComponent);

   static void CheckLightEmitterAgainstDetector(const UOSELightDetectionComponent* lightDetectionComponent,
                                                const UPrimitiveComponent* component,
                                                const IOSELightDetectionInterface* lightDetectionInterface,
                                                int32& numberOfLoSChecksPerformed,
                                                float& calculatedLightIntensity,
                                                FLinearColor& calculatedLightColor,
                                                int& totalLightsHittingDetector,
                                                const UOSELightEmittingComponent* lightEmittingComponent,
                                                const UOSELightDetectionSettings& lightDetectionSettings);
   
   virtual void Tick(float deltaTime) override;
   virtual TStatId GetStatId() const override;

private:
   UPROPERTY(Transient)
   TArray<TObjectPtr<UOSELightDetectionComponent>> LightDetectionComponents;

   UPROPERTY(Transient)
   TArray<TObjectPtr<UOSELightEmittingComponent>> MovableLightEmittingComponents;

   UPROPERTY(Transient)
   TMap<FLightEmitterHandle, FLightEmitterRuntimeData> HandleToLightEmitterRuntimeDataMap;
   
   FLightEmitterOctree StaticLightEmitterOctree;

   int _numberOfRegisteredLights { 0 };
};
