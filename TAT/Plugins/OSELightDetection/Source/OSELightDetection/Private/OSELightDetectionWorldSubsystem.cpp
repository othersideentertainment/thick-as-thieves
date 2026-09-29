// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSELightDetectionWorldSubsystem.h"

// ose
#include "Math/OSEMathFunctionLibrary.h"
#include "OSELightDetectionComponent.h"
#include "OSELightDetectionInterface.h"
#include "OSELightDetectionSettings.h"
#include "OSELightEmittingComponent.h"

// ue
#include "Components/DirectionalLightComponent.h"
#include "LevelUtils.h"
#include "Engine/LevelBounds.h"
#include "Engine/LevelStreaming.h"
#include "WorldPartition/WorldPartitionLevelStreamingDynamic.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(OSELightDetectionWorldSubsystem)

DECLARE_STATS_GROUP(TEXT("LightDetection"),STATGROUP_LightDetection, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Time to check"), STAT_LightDetection_Tick, STATGROUP_LightDetection)
DECLARE_CYCLE_STAT(TEXT("Time to check - detection"), STAT_LightDetection_Tick_Detection, STATGROUP_LightDetection)
DECLARE_CYCLE_STAT(TEXT("Time to check - detection - check light emitter"), STAT_LightDetection_Tick_Detection_CheckLightEmitter, STATGROUP_LightDetection)
DECLARE_DWORD_COUNTER_STAT(TEXT("Lights Registered"), STAT_LightDetection_LightsRegistered, STATGROUP_LightDetection);
DECLARE_DWORD_COUNTER_STAT(TEXT("Light LoS Checks Performed"), STAT_LightDetection_LightLoSChecks, STATGROUP_LightDetection);
DECLARE_DWORD_COUNTER_STAT(TEXT("Light Static Emitters Checked"), STAT_LightDetection_StaticLightEmittersChecked, STATGROUP_LightDetection);
DECLARE_DWORD_COUNTER_STAT(TEXT("Light Mobile Emitters Checked"), STAT_LightDetection_MobileLightEmittersChecked, STATGROUP_LightDetection);

namespace LightDetectionCVars
{
   static int32 EnableLightDetection = 1;
   FAutoConsoleVariableRef CVarEnableLightDetection(
      TEXT("tat.AI.EnableLightDetection"),
      EnableLightDetection,
      TEXT("Enable light detection?"),
      ECVF_Default);
   static int32 DumpLightDetectionOctree = 0;
   FAutoConsoleVariableRef CVarDumpLightDetectionOctree(
      TEXT("tat.AI.DumpLightDetectionOctree"),
      DumpLightDetectionOctree,
      TEXT("Dump Octree for lighting?"),
      ECVF_Default);
   static int32 EnableDebugBoundsForCheckedLights = 0;
   FAutoConsoleVariableRef CVarEnableDebugBoundsForCheckedLights(
      TEXT("tat.AI.EnableDebugBoundsForCheckedLights"),
      EnableDebugBoundsForCheckedLights,
      TEXT("Should we display bounding boxes for lights we are checking?"),
      ECVF_Default);
   static int32 EnableDebugTextForLightIntensity = 0;
   FAutoConsoleVariableRef CVarEnableDebugTextForLightIntensity(
      TEXT("tat.AI.EnableDebugTextForLightIntensity"),
      EnableDebugTextForLightIntensity,
      TEXT("Should we display the light intensity values over the lights?"),
      ECVF_Default);
}


struct FLightEmitterHandleFactory
{
   //Mirroring the FSmartObjectHandleFactory implementation
   static FLightEmitterHandle CreateLightEmitterHandle(const UWorld& world, const UOSELightEmittingComponent& component)
   {
      const FSoftObjectPath objectPath = &component;
      FString assetPathString = objectPath.GetAssetPathString();

      bool bIsStreamedByWorldPartition = false;
      if (world.IsPartitionedWorld())
      {
         if (const AActor* ownerActor = component.GetOwner())
         {
            if (const ULevelStreaming* baseLevelStreaming = FLevelUtils::FindStreamingLevel(ownerActor->GetLevel()))
            {
               bIsStreamedByWorldPartition = baseLevelStreaming->IsA<UWorldPartitionLevelStreamingDynamic>();
            }
         }
      }

      // We are not using asset path for partitioned world since they are not stable between editor and runtime.
      // SubPathString should be enough since all actors are part of the main level.
      if (bIsStreamedByWorldPartition)
      {
         assetPathString.Reset();
      }
#if WITH_EDITOR
      else if (world.WorldType == EWorldType::PIE)
      {
         assetPathString = UWorld::RemovePIEPrefix(objectPath.GetAssetPathString());
      }
#endif // WITH_EDITOR

      // Compute hash manually from strings since GetTypeHash(FSoftObjectPath) relies on a FName which implements run-dependent hash computations.
      return FLightEmitterHandle(HashCombine(GetTypeHash(assetPathString), GetTypeHash(objectPath.GetSubPathString())));
   }
};

TStatId UOSELightDetectionWorldSubsystem::GetStatId() const
{
   RETURN_QUICK_DECLARE_CYCLE_STAT(UOSELightDetectionWorldSubsystem, STATGROUP_Tickables);
}

void UOSELightDetectionWorldSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);
}

void UOSELightDetectionWorldSubsystem::OnWorldBeginPlay(UWorld& inWorld)
{
   FBox worldBounds(ForceInitToZero);
   if (const ULevel* persistentLevel = inWorld.PersistentLevel.Get())
   {
      if (persistentLevel->LevelBoundsActor.IsValid())
      {
         worldBounds = persistentLevel->LevelBoundsActor.Get()->GetComponentsBoundingBox();
      }
      else
      {
         worldBounds = ALevelBounds::CalculateLevelBounds(persistentLevel);
      }
   }
   StaticLightEmitterOctree = FLightEmitterOctree(worldBounds.GetCenter(), worldBounds.GetExtent().Size2D());
   Super::OnWorldBeginPlay(inWorld);
}

void UOSELightDetectionWorldSubsystem::OnWorldComponentsUpdated(UWorld& world)
{
   Super::OnWorldComponentsUpdated(world);
}

void UOSELightDetectionWorldSubsystem::RegisterLight(UOSELightEmittingComponent* lightEmittingComponent)
{
   ++_numberOfRegisteredLights;
   if(lightEmittingComponent->IsMovable())
   {
      MovableLightEmittingComponents.Add(lightEmittingComponent);
   }
   else
   {      
      FLightEmitterRuntimeData runtimeData;
      runtimeData.OwnerComponent = lightEmittingComponent;
      runtimeData.RegisteredHandle = FLightEmitterHandleFactory::CreateLightEmitterHandle(*lightEmittingComponent->GetWorld(), *lightEmittingComponent);

      lightEmittingComponent->SetRegisteredHandle(runtimeData.RegisteredHandle);
      StaticLightEmitterOctree.AddNode(lightEmittingComponent->GetLightBounds(), runtimeData.RegisteredHandle, runtimeData.SpatialEntryData.SharedOctreeID);
      HandleToLightEmitterRuntimeDataMap.Add(runtimeData.RegisteredHandle, runtimeData);
   }
}

void UOSELightDetectionWorldSubsystem::UnRegisterLight(UOSELightEmittingComponent* lightEmittingComponent)
{
   --_numberOfRegisteredLights;
   if(lightEmittingComponent->IsMovable())
   {
      check(lightEmittingComponent->GetRegisteredHandle().IsValid() == false);
      MovableLightEmittingComponents.Remove(lightEmittingComponent);
   }
   else
   {
      const FLightEmitterHandle handle = lightEmittingComponent->GetRegisteredHandle();
      check(handle.IsValid());
      
      const FLightEmitterRuntimeData* runtimeData = HandleToLightEmitterRuntimeDataMap.Find(handle);
      if(runtimeData == nullptr)
      {
         checkf(false, TEXT("Handle to light emitter is invalid!"))
         return;
      }

      FLightEmitterOctreeID lightEmitterOctreeID = runtimeData->SpatialEntryData.SharedOctreeID.Get();
      if(lightEmitterOctreeID.ID.IsValidId() == false)
      {
         checkf(false, TEXT("OctreeID is invalid for light emitter runtime data!"))
         return;
      }
      StaticLightEmitterOctree.RemoveNode(lightEmitterOctreeID.ID);
      lightEmitterOctreeID.ID = {};

      HandleToLightEmitterRuntimeDataMap.Remove(handle);
   }
}

void UOSELightDetectionWorldSubsystem::RegisterLightDetectionComponent(
   UOSELightDetectionComponent* lightDetectionComponent)
{
   LightDetectionComponents.Add(lightDetectionComponent);
}

void UOSELightDetectionWorldSubsystem::UnRegisterLightDetectionComponent(
   UOSELightDetectionComponent* lightDetectionComponent)
{
   LightDetectionComponents.Remove(lightDetectionComponent);
}

void UOSELightDetectionWorldSubsystem::CheckLightEmitterAgainstDetector(
   const UOSELightDetectionComponent* lightDetectionComponent,
   const UPrimitiveComponent* component,
   const IOSELightDetectionInterface* lightDetectionInterface,
   int32& numberOfLoSChecksPerformed,
   float& calculatedLightIntensity,
   FLinearColor& calculatedLightColor,
   int& totalLightsHittingDetector,
   const UOSELightEmittingComponent* lightEmittingComponent,
   const UOSELightDetectionSettings& lightDetectionSettings)
{
   SCOPE_CYCLE_COUNTER(STAT_LightDetection_Tick_Detection_CheckLightEmitter)
   if(lightEmittingComponent->AffectsDetectionComponent(component))
   {
#if ENABLE_DRAW_DEBUG
      if(LightDetectionCVars::EnableDebugBoundsForCheckedLights != 0)
      {
         const FBox bounds = lightEmittingComponent->GetLightBounds();
         DrawDebugBox(component->GetWorld(), bounds.GetCenter(), bounds.GetExtent(), lightEmittingComponent->GetLightColor().ToFColor(false), false, 0.2f, 0, 0);
      }      
#endif
      float minHitDistance { 0 };
      if (lightDetectionInterface->CanLightRayHitActor(
         lightEmittingComponent->GetLightEmissionLocation(lightDetectionComponent->GetLightDetectionTransform()),
         lightEmittingComponent->GetOwner(),
         numberOfLoSChecksPerformed,
         minHitDistance))
      {
         float lightIntensity;
         // For directional lights, we ignore their attenuation (as it is infinite)
         const ULightComponent* lightComponent = lightEmittingComponent->_lightComponent;
         if (lightComponent->IsA<UDirectionalLightComponent>())
         {
            // Clamp directional light intensity to valid lux-range
            const FLightIntensityConfig& directionalLightSettings = lightDetectionSettings.DirectionalLightSettings;
            const FFloatRange luxRange = directionalLightSettings.LuxRange;
            lightIntensity = FMath::Clamp(lightComponent->Intensity, luxRange.GetLowerBoundValue(), luxRange.GetUpperBoundValue());
            if (directionalLightSettings.RemapLightIntensityToRange)
            {
               // Get the intensity's percentage of the lux range, then interpolate that to the remap-range
               const float alpha = FMath::GetMappedRangeValueClamped(luxRange, FFloatRange(0,1), lightIntensity);
               const FFloatRange remapRange = directionalLightSettings.RemapRange;
               lightIntensity = UOSEMathFunctionLibrary::Interpolate(remapRange.GetLowerBoundValue(), remapRange.GetUpperBoundValue(), alpha, directionalLightSettings.IntensityInterpMode);
            }
         }
         else
         {
            const float lightBrightness = lightComponent->ComputeLightBrightness() * lightEmittingComponent->GetLightFudgeValue();
            const float distancedSquared = FMath::Max(FMath::Square(minHitDistance), 0.01f);
            lightIntensity = lightBrightness / distancedSquared;
#if ENABLE_DRAW_DEBUG
            if(LightDetectionCVars::EnableDebugTextForLightIntensity != 0)
            {
               DrawDebugString(component->GetWorld(),
                  FVector(.0f, .0f,  .0f),
                  FString::Printf(TEXT("Brightness: %f, Distance sqr: %f, Actual Intensity %f"),
                     lightBrightness,
                     distancedSquared,
                     lightIntensity),
                     lightEmittingComponent->GetOwner(),
                     FColor::White,
                     0.f,
                     true);
            }      
#endif
         }
         calculatedLightColor += lightEmittingComponent->GetLightColor();
         calculatedLightIntensity = FMath::Max(lightIntensity, calculatedLightIntensity);
         totalLightsHittingDetector++;
      }
   }
}

void UOSELightDetectionWorldSubsystem::Tick(const float deltaTime)
{
   Super::Tick(deltaTime);
   if(LightDetectionCVars::EnableLightDetection == 0)
      return;
   const UOSELightDetectionSettings& lightDetectionSettings = UOSELightDetectionSettings::Get();
   SCOPE_CYCLE_COUNTER(STAT_LightDetection_Tick)
   int32 numberOfLoSChecksPerformed { 0 };
   int32 numberOfStaticLightEmittersChecked { 0 };
   int32 numberOfMobileLightEmittersChecked { 0 };

      
   //TODO: Time slice this based on max number of traces per frame so we can control the cost of this calculation
   //It doesn't have to be _instant_ but it has to be responsive enough to work with.
   for (const TObjectPtr<UOSELightDetectionComponent>& lightDetectionComponent : LightDetectionComponents)
   {
      SCOPE_CYCLE_COUNTER(STAT_LightDetection_Tick_Detection)
      if(lightDetectionComponent->CanHandleCalculation() == false)
         continue;
      const UPrimitiveComponent* component = lightDetectionComponent->GetPrimitiveComponent();
      if(component == nullptr)
         continue;
      const IOSELightDetectionInterface* lightDetectionInterface = lightDetectionComponent->GetLightDetectionInterface();
      if(lightDetectionInterface == nullptr)
         continue;

      float calculatedLightIntensity = 0.f;
      FLinearColor calculatedLightColor { FLinearColor(calculatedLightIntensity, calculatedLightIntensity,calculatedLightIntensity, 1.f) };
      int totalLightsHittingDetector { 0 };
      
      // 1. Check Movable Lights
      for (const TObjectPtr<UOSELightEmittingComponent>& lightEmittingComponent : MovableLightEmittingComponents)
      {
         CheckLightEmitterAgainstDetector(lightDetectionComponent, component, lightDetectionInterface,
                                          numberOfLoSChecksPerformed, calculatedLightIntensity, calculatedLightColor,
                                          totalLightsHittingDetector, lightEmittingComponent, lightDetectionSettings);
         ++numberOfMobileLightEmittersChecked;
      }
      
      TArray<FLightEmitterHandle> lightEmitterHandles;
      // 2. Check static lights in the octree
      StaticLightEmitterOctree.FindElementsWithBoundsTest(component->Bounds.GetBox(),
      [&lightEmitterHandles](const FLightEmitterOctreeElement& element)
      {
         lightEmitterHandles.Add(element.LightEmitterHandle);
      });
      
      for (auto lightEmitterHandle : lightEmitterHandles)
      {
         const FLightEmitterRuntimeData& lightRuntimeData = HandleToLightEmitterRuntimeDataMap.FindChecked(lightEmitterHandle);
         CheckLightEmitterAgainstDetector(
            lightDetectionComponent,
            component,
            lightDetectionInterface,
            numberOfLoSChecksPerformed,
            calculatedLightIntensity,
            calculatedLightColor,
            totalLightsHittingDetector,
            lightRuntimeData.OwnerComponent.Get(),
            lightDetectionSettings);
         ++numberOfStaticLightEmittersChecked;
      }
      float preClampedValue = calculatedLightIntensity;
      if(lightDetectionSettings.ShouldClampLightBrightness)
      {
         for (const FLightIntensityClampingConfig& config : lightDetectionSettings.LightClampingConfig)
         {
            if(config.InputRange.Contains(calculatedLightIntensity))
            {
               preClampedValue = config.OutputValue;
            }
         }
         calculatedLightIntensity = preClampedValue;
      }
      const float minimumLightDetectionValue = lightDetectionComponent->GetMinimumLightIntensity();
      lightDetectionComponent->SetLightDetectionValues(
         FMath::Clamp(preClampedValue, 0.f, 1.f),
         FMath::Clamp(FMath::Max(minimumLightDetectionValue, calculatedLightIntensity), 0.f, 1.f),
            calculatedLightColor);
   }
   
   SET_CYCLE_COUNTER(STAT_LightDetection_LightLoSChecks, numberOfLoSChecksPerformed);
   SET_CYCLE_COUNTER(STAT_LightDetection_MobileLightEmittersChecked, numberOfMobileLightEmittersChecked);
   SET_CYCLE_COUNTER(STAT_LightDetection_StaticLightEmittersChecked, numberOfStaticLightEmittersChecked);
   SET_CYCLE_COUNTER(STAT_LightDetection_LightsRegistered, _numberOfRegisteredLights);
   
   
   if(LightDetectionCVars::DumpLightDetectionOctree == 1)
   {
      LightDetectionCVars::DumpLightDetectionOctree = 0;
      StaticLightEmitterOctree.DumpStats();
   }
}
