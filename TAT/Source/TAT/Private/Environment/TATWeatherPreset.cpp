// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Environment/TATWeatherPreset.h"

// tat
#include "Developer/TATWeatherSettings.h"
#include "Environment/TATWeatherManager.h"
#include "GameFramework/TATWorldSettings.h"

// ose
#include "OSECoreCheats.h"

// ue
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "NiagaraParameterCollection.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Engine/AssetManager.h"
#include "Materials/MaterialParameterCollection.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWeatherPreset)

#define LOCTEXT_NAMESPACE "TATWeatherPreset"

DEFINE_LOG_CATEGORY_STATIC(LogTATWeatherPreset, Log, All);

DECLARE_CYCLE_STAT(TEXT("Weather Preset: Tick (Total)"), STAT_WeatherPresetTick, STATGROUP_Weather);
DECLARE_CYCLE_STAT(TEXT("Weather Preset: Tick (Rain)"), STAT_WeatherPresetTick_Rain, STATGROUP_Weather);
DECLARE_CYCLE_STAT(TEXT("Weather Preset: Tick (Wind)"), STAT_WeatherPresetTick_Wind, STATGROUP_Weather);
DECLARE_CYCLE_STAT(TEXT("Weather Preset: Tick (Poll Nearby Physical Surfaces)"), STAT_WeatherPresetTick_PollPhysSurfaces, STATGROUP_Weather);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Weather Preset: Num Tracked Physical Surfaces"), STAT_WeatherPreset_NumTrackedPhysicalSurfaces, STATGROUP_Weather);

namespace WeatherHelpers
{
   inline void SetPhysicalSurfaceDebugMessage(UPhysicalMaterial* physicalMaterial, const FString& message, FColor color, float timeToDisplay = 3.0f)
   {
      if (physicalMaterial == nullptr)
      {
         return;
      }
      const FName physmatName = physicalMaterial->GetFName();
      GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetTypeHash(physmatName)), timeToDisplay, color, FString::Printf(TEXT("PhysicalMaterial(%s): %s"),
         *physmatName.ToString(), *message));
   }
}

ATATWeatherPreset::ATATWeatherPreset()
{
   RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("WeatherPresetRootComponent"));
   RootComponent->SetMobility(EComponentMobility::Movable);

   PrimaryDirectionalLightComponent = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("PrimaryDirectionalLightComponent"));
   PrimaryDirectionalLightComponent->SetRelativeLocation(FVector(0, 0, 100.0f));
   PrimaryDirectionalLightComponent->SetupAttachment(RootComponent);

   HeightFogComponent = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFogComponent"));
   HeightFogComponent->SetRelativeLocation(FVector(0, 0, 300.0f));
   HeightFogComponent->SetupAttachment(RootComponent);

   SkyLightComponent = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLightComponent"));
   SkyLightComponent->SetupAttachment(RootComponent);
   SkyLightComponent->SetRelativeLocation(FVector(0, 0, 200.0f));
   SkyLightComponent->CastShadows = false;

   SkyAtmosphereComponent = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphereComponent"));
   SkyAtmosphereComponent->SetupAttachment(RootComponent);
   SkyAtmosphereComponent->SetRelativeLocation(FVector(0, 0, 400.0f));

   PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
   PostProcessComponent->SetupAttachment(RootComponent);
   PostProcessComponent->bUnbound = true;

#if WITH_EDITORONLY_DATA
   PrimaryDirectionalLightArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("PrimaryDirectionalLightArrowComponent"));
   if (PrimaryDirectionalLightArrowComponent != nullptr)
   {
      PrimaryDirectionalLightArrowComponent->SetArrowColor(FLinearColor(FColor(191, 220, 227)));
      PrimaryDirectionalLightArrowComponent->SetupAttachment(PrimaryDirectionalLightComponent);
   }

   HeightFogSpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("HeightFogSpriteComponent"));
   if (HeightFogSpriteComponent != nullptr)
   {
      HeightFogSpriteComponent->SetRelativeScale3D_Direct(FVector(0.5f, 0.5f, 0.5f));
      HeightFogSpriteComponent->SpriteInfo.Category = FName("Fog");
      HeightFogSpriteComponent->SpriteInfo.DisplayName = LOCTEXT("Fog", "Fog");
      if (!IsRunningCommandlet())
      {
         static ConstructorHelpers::FObjectFinderOptional<UTexture2D> fogTexture = TEXT("/Engine/EditorResources/S_ExpoHeightFog");
         HeightFogSpriteComponent->Sprite = fogTexture.Get();
      }
      HeightFogSpriteComponent->SetupAttachment(HeightFogComponent);
   }
#endif

   RainNiagaraComponent = CreateOptionalDefaultSubobject<UNiagaraComponent>(TEXT("RainNiagaraComponent"));
   if (RainNiagaraComponent != nullptr)
   {
      RainNiagaraComponent->SetAutoActivate(false);
      RainNiagaraComponent->SetupAttachment(RootComponent);
   }

   WindNiagaraComponent = CreateOptionalDefaultSubobject<UNiagaraComponent>(TEXT("WindNiagaraComponent"));
   if (WindNiagaraComponent != nullptr)
   {
      WindNiagaraComponent->SetAutoActivate(false);
      WindNiagaraComponent->SetupAttachment(RootComponent);
   }

   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = false;
   PrimaryActorTick.bAllowTickOnDedicatedServer = false;
}

void ATATWeatherPreset::OnConstruction(const FTransform& transform)
{
#if WITH_EDITOR
   // For editor previews, setup rain and wind on construction to allow live previews
   if (!HasAnyFlags(RF_ClassDefaultObject) && GEditor && !GEditor->IsPlayingSessionInEditor())
   {
      // In the editor we can't always get away with manually activating niagara components for some reason.
      // Just set them to auto-activate if they have a valid particle system asset and their respective systems are enabled.
      // This is a bit of a hack to enable auto activate only in the editor without modifying the preset's CDO, but it's editor-only so I'm not too worried about it.
      auto resetComponentAutoActivation = [](UNiagaraComponent* comp, bool autoActivate, const TSoftObjectPtr<UNiagaraSystem>& defaultAsset)
      {
         if (comp == nullptr)
         {
            return;
         }
         // Technically we can call SetAutoActivate without unregistering and reregistering the component, but it causes a lot of spammy warnings.
         const bool wasRegistered = comp->IsRegistered();
         if (wasRegistered)
         {
            comp->UnregisterComponent();
         }
         // If we're trying to activate the particle system but it doesn't have an assigned niagara asset, set the default one.
         if (autoActivate && comp->GetAsset() == nullptr && !defaultAsset.IsNull())
         {
            comp->SetAsset(defaultAsset.LoadSynchronous());
         }
         comp->SetAutoActivate(autoActivate);
         if (wasRegistered)
         {
            comp->RegisterComponent();
         }
      };

      resetComponentAutoActivation(RainNiagaraComponent, IsRainEnabled(), UTATWeatherSettings::Get().DefaultRainParticleSystem);
      resetComponentAutoActivation(WindNiagaraComponent, IsWindEnabled(), UTATWeatherSettings::Get().DefaultWindParticleSystem);

      OnSetupRain();
      OnSetupWind();
   }
#endif

   Super::OnConstruction(transform);
}

void ATATWeatherPreset::BeginPlay()
{
   Super::BeginPlay();

   if (GetWorld()->IsNetMode(NM_DedicatedServer))
   {
      SetActorTickEnabled(false);
      return;
   }

   // Unless a subclass has set bStartWithTickEnabled to true, only enable tick if rain or wind is enabled
   if (!PrimaryActorTick.bStartWithTickEnabled)
   {
      SetActorTickEnabled((UpdateRainOnTick && IsRainEnabled()) || (UpdateWindOnTick && IsWindEnabled()));
   }

   // Pre-populate the nearby physical surfaces map
   if (TrackAllPhysicalSurfaceTypes)
   {
      _nearbyPhysicalSurfaces.Reserve(32);
   }
   else
   {
      _nearbyPhysicalSurfaces.Reserve(TrackNearbyPhysicalSurfaceTypes.Num());
      for (UPhysicalMaterial* physicalMaterial : TrackNearbyPhysicalSurfaceTypes)
      {
         _nearbyPhysicalSurfaces.Add(physicalMaterial, FTATNearbyPhysicalSurface());
      }
   }

   // Always call these setup functions even if disabled so they can set up state correctly
   OnSetupRain();
   OnSetupWind();
}

void ATATWeatherPreset::Tick(float deltaSeconds)
{
   Super::Tick(deltaSeconds);

   SCOPE_CYCLE_COUNTER(STAT_WeatherPresetTick);

   if (UpdateRainOnTick && IsRainEnabled())
   {
      SCOPE_CYCLE_COUNTER(STAT_WeatherPresetTick_Rain);
      OnTickRain(deltaSeconds);
   }

   if (UpdateWindOnTick && IsWindEnabled())
   {
      SCOPE_CYCLE_COUNTER(STAT_WeatherPresetTick_Wind);
      OnTickWind(deltaSeconds);
   }

   if (_nearbyPhysicalSurfaces.Num() > 0 || TrackAllPhysicalSurfaceTypes)
   {
      _PollNearbyOutsidePhysicalSurfaces();
   }

#if OSE_CHEATS_ENABLED
   ATATWeatherManager* weatherManager = _GetWeatherManager();
   if (weatherManager != nullptr && weatherManager->GetTemporalTraceDebuggingEnabled())
   {
      FVector cameraViewpoint;
      if (GetCameraViewpoint(cameraViewpoint))
      {
         static constexpr int32 messageKey = 42;
         GEngine->AddOnScreenDebugMessage(messageKey - 1, 1.0f, FColor::White, TEXT("Temporal Environment Trace System"));

         if (weatherManager->IsCameraViewpointInside())
         {
            FVector outsideLocation;
            if (FindClosestKnownOutsideWorldLocationToCameraViewpoint(outsideLocation))
            {
               const FVector offset = FVector(0, 0, -35.0f);
               const FVector direction = (outsideLocation - cameraViewpoint).GetSafeNormal();
               DrawDebugDirectionalArrow(GetWorld(), cameraViewpoint + offset, cameraViewpoint + offset + (direction * 200.0f), 40.0f, FColor::Yellow);

               GEngine->AddOnScreenDebugMessage(messageKey, 1.0f, FColor::Yellow, FString::Printf(TEXT("Currently INSIDE; distance to outside point = %.2f"),
                  FVector::Distance(outsideLocation, cameraViewpoint)));
            }
         }
         else
         {
            GEngine->AddOnScreenDebugMessage(messageKey, 1.0f, FColor(255, 190, 255), TEXT("Currently OUTSIDE"));
         }

         for (const auto& pair : _nearbyPhysicalSurfaces)
         {
            if (pair.Value.IsValid)
            {
               const float dist = FVector::Distance(cameraViewpoint, pair.Value.WorldLocation);
               WeatherHelpers::SetPhysicalSurfaceDebugMessage(pair.Key, FString::Printf(TEXT("distance = %.2f"), dist), FColor::Cyan);
            }
         }
      }
   }
#endif
}

bool ATATWeatherPreset::GetCameraViewpoint(FVector& cameraViewpoint) const
{
   if (ATATWeatherManager* weatherManager = _GetWeatherManager())
   {
      return weatherManager->GetCameraViewpoint(cameraViewpoint);
   }
   cameraViewpoint = FVector::ZeroVector;
   return false;
}

bool ATATWeatherPreset::IsRainEnabled() const
{
   return EnableRain && RainIntensity > 0;
}

bool ATATWeatherPreset::IsWindEnabled() const
{
   return EnableWind && UTATWeatherSettings::Get().MaxWindStrength > 0;
}

float ATATWeatherPreset::CalcWindStrength(float worldTimeSeconds) const
{
   return FMath::Clamp(WindStrength * WindStrengthTurbulence.Evaluate(worldTimeSeconds), 0.0f, UTATWeatherSettings::Get().MaxWindStrength);
}

FVector ATATWeatherPreset::CalcWindDirection(float worldTimeSeconds) const
{
   const FQuat baseWindOrientation = WindDirection.Quaternion();
   return baseWindOrientation.GetForwardVector() + (baseWindOrientation.GetRightVector() * WindDirectionTurbulence.Evaluate(worldTimeSeconds));
}

void ATATWeatherPreset::SetGlobalMaterialParameterInt(FName paramName, int32 value)
{
   if (paramName == NAME_None)
   {
      return;
   }
   UKismetMaterialLibrary::SetScalarParameterValue(this, _GetMaterialParameterCollection(), paramName, static_cast<float>(value));
}

void ATATWeatherPreset::SetGlobalMaterialParameterFloat(FName paramName, float value)
{
   if (paramName == NAME_None)
   {
      return;
   }
   UKismetMaterialLibrary::SetScalarParameterValue(this, _GetMaterialParameterCollection(), paramName, value);
}

void ATATWeatherPreset::SetGlobalMaterialParameterVector2D(FName paramName, const FVector2D& value)
{
   if (paramName == NAME_None)
   {
      return;
   }
   UKismetMaterialLibrary::SetVectorParameterValue(this, _GetMaterialParameterCollection(), paramName, FLinearColor(value.X, value.Y, 0.0f, 0.0f));
}

void ATATWeatherPreset::SetGlobalMaterialParameterVector(FName paramName, const FVector& value)
{
   if (paramName == NAME_None)
   {
      return;
   }
   UKismetMaterialLibrary::SetVectorParameterValue(this, _GetMaterialParameterCollection(), paramName, FLinearColor(value.X, value.Y, value.Z, 0.0f));
}

void ATATWeatherPreset::SetGlobalMaterialParameterColor(FName paramName, const FLinearColor& value)
{
   if (paramName == NAME_None)
   {
      return;
   }
   UKismetMaterialLibrary::SetVectorParameterValue(this, _GetMaterialParameterCollection(), paramName, value);
}

// void ATATWeatherPreset::SetGlobalMaterialParameterTexture(FName paramName, UTexture* value)
// {
//    if (paramName == NAME_None)
//    {
//       return;
//    }
// }

void ATATWeatherPreset::SetGlobalNiagaraParameterInt(FName paramName, int32 value)
{
   if (paramName == NAME_None)
   {
      return;
   }
   if (UNiagaraParameterCollectionInstance* params = UNiagaraFunctionLibrary::GetNiagaraParameterCollection(this, _GetNiagaraParameterCollection()))
   {
      params->SetIntParameter(paramName.ToString(), value);
   }
}

void ATATWeatherPreset::SetGlobalNiagaraParameterFloat(FName paramName, float value)
{
   if (paramName == NAME_None)
   {
      return;
   }
   if (UNiagaraParameterCollectionInstance* params = UNiagaraFunctionLibrary::GetNiagaraParameterCollection(this, _GetNiagaraParameterCollection()))
   {
      params->SetFloatParameter(paramName.ToString(), value);
   }
}

void ATATWeatherPreset::SetGlobalNiagaraParameterVector2D(FName paramName, const FVector2D& value)
{
   if (paramName == NAME_None)
   {
      return;
   }
   if (UNiagaraParameterCollectionInstance* params = UNiagaraFunctionLibrary::GetNiagaraParameterCollection(this, _GetNiagaraParameterCollection()))
   {
      params->SetVector2DParameter(paramName.ToString(), value);
   }
}

void ATATWeatherPreset::SetGlobalNiagaraParameterVector(FName paramName, const FVector& value)
{
   if (paramName == NAME_None)
   {
      return;
   }
   if (UNiagaraParameterCollectionInstance* params = UNiagaraFunctionLibrary::GetNiagaraParameterCollection(this, _GetNiagaraParameterCollection()))
   {
      params->SetVectorParameter(paramName.ToString(), value);
   }
}

void ATATWeatherPreset::SetGlobalNiagaraParameterColor(FName paramName, const FLinearColor& value)
{
   if (paramName == NAME_None)
   {
      return;
   }
   if (UNiagaraParameterCollectionInstance* params = UNiagaraFunctionLibrary::GetNiagaraParameterCollection(this, _GetNiagaraParameterCollection()))
   {
      params->SetColorParameter(paramName.ToString(), value);
   }
}

void ATATWeatherPreset::SetGlobalNiagaraParameterTexture(FName paramName, UTexture* value)
{
   if (paramName == NAME_None)
   {
      return;
   }
   constexpr bool logOnError = true;
   UTATWeatherUtilities::SetNiagaraParameterCollectionTexture(this, _GetNiagaraParameterCollection(), paramName, value, logOnError);
}

bool ATATWeatherPreset::UpdateCurrentWindDirectionAndStrength(const FVector& newWindDirection, float newWindStrength, bool forceUpdate)
{
   const FVector prevWindDirection = _currentWindDirection;
   const float prevWindStrength = _currentWindStrength;

   _currentWindDirection = newWindDirection;
   _currentWindStrength = newWindStrength;

   const bool windDirectionUpdated = forceUpdate || !prevWindDirection.Equals(newWindDirection);
   const bool windStrengthUpdated = forceUpdate || !FMath::IsNearlyEqual(prevWindStrength, newWindStrength);

   const UTATWeatherSettings& weatherSettings = UTATWeatherSettings::Get();

   if (windDirectionUpdated)
   {
      SetGlobalMaterialAndNiagaraParameter<FVector>(
         weatherSettings.MaterialParameterNames.WindDirection,
         weatherSettings.NiagaraParameterNames.WindDirection,
         _currentWindDirection);
   }

   if (windStrengthUpdated)
   {
      SetGlobalMaterialAndNiagaraParameter<float>(
         weatherSettings.MaterialParameterNames.WindStrength,
         weatherSettings.NiagaraParameterNames.WindStrength,
         _currentWindStrength);
      SetGlobalMaterialAndNiagaraParameter<float>(
         weatherSettings.MaterialParameterNames.WindStrengthNormalized,
         weatherSettings.NiagaraParameterNames.WindStrengthNormalized,
         UTATWeatherUtilities::NormalizeWindStrength(_currentWindStrength));
   }

   if (windDirectionUpdated || windStrengthUpdated)
   {
      if (ATATWeatherManager* weatherManager = _GetWeatherManager())
      {
         weatherManager->UpdateWindDirectionAndStrengthFromPreset(this, _currentWindDirection, _currentWindStrength);
      }
   }

   return true;
}

void ATATWeatherPreset::OnSetupRain_Implementation()
{
   const bool rainEnabled = IsRainEnabled();

   const UTATWeatherSettings& weatherSettings = UTATWeatherSettings::Get();

   SetGlobalMaterialAndNiagaraParameter<float>(
      weatherSettings.MaterialParameterNames.RainIntensity,
      weatherSettings.NiagaraParameterNames.RainIntensity,
      rainEnabled ? RainIntensity : 0.0f);

   SetGlobalMaterialAndNiagaraParameter<float>(
      weatherSettings.MaterialParameterNames.GroundWetness,
      weatherSettings.NiagaraParameterNames.GroundWetness,
      rainEnabled ? GroundWetness : 0.0f);

   // Always init camera wetness to zero and let the OnCameraViewportIndoorStateChange event take care of updating it
   SetGlobalMaterialAndNiagaraParameter<float>(
      weatherSettings.MaterialParameterNames.CameraWetness,
      weatherSettings.NiagaraParameterNames.CameraWetness,
      0.0f);

   _TrySetParticleComponentActivated(RainNiagaraComponent, rainEnabled, weatherSettings.DefaultRainParticleSystem, TEXT("rain"));
}

void ATATWeatherPreset::OnTickRain_Implementation(float deltaSeconds)
{
}

void ATATWeatherPreset::OnSetupWind_Implementation()
{
   FVector direction = FVector::ZeroVector;
   float strength = 0.0f;
   _CalcCurrentWindDirectionAndStrength(direction, strength);
   constexpr bool forceUpdate = true;
   UpdateCurrentWindDirectionAndStrength(direction, strength, forceUpdate);

   SetGlobalNiagaraParameterFloat(UTATWeatherSettings::Get().NiagaraParameterNames.WindDebrisAmount, WindDebrisAmount);

   const bool windDebrisParticlesActivated = IsWindEnabled() && WindDebrisAmount > 0;
   _TrySetParticleComponentActivated(WindNiagaraComponent, windDebrisParticlesActivated, UTATWeatherSettings::Get().DefaultWindParticleSystem, TEXT("wind"));
}

void ATATWeatherPreset::OnTickWind_Implementation(float deltaSeconds)
{
   FVector direction = FVector::ZeroVector;
   float strength = 0.0f;
   _CalcCurrentWindDirectionAndStrength(direction, strength);
   UpdateCurrentWindDirectionAndStrength(direction, strength);
}

void ATATWeatherPreset::OnSceneDepthTextureUpdated_Implementation(UTextureRenderTarget2D* renderTarget, const FBox& weatherWorldBoundingBox)
{
   check(renderTarget != nullptr);
   float depthmapWorldSize = 0.0f;
   float depthmapWorldHeight = 0.0f;
   FVector depthmapOrigin = FVector::ZeroVector;
   ATATWeatherManager::CalcDepthmapParameters(weatherWorldBoundingBox, depthmapWorldSize, depthmapWorldHeight, depthmapOrigin);
   ensure(depthmapWorldSize > 0);
   ensure(depthmapWorldHeight > 0);
   UTATWeatherUtilities::SetWeatherDepthmapParameters(
      this,
      _GetMaterialParameterCollection(),
      _GetNiagaraParameterCollection(),
      renderTarget,
      depthmapWorldSize,
      depthmapWorldHeight,
      depthmapOrigin);
}

void ATATWeatherPreset::OnCameraViewportIndoorStateChange_Implementation(bool newIndoorState, const FVector& cameraViewpoint)
{
   // Set the CameraWetness parameter based on rain settings and indoor state
   const float newCameraWetness = (EnableRain && !newIndoorState) ? CameraWetness : 0.0f;
   const UTATWeatherSettings& weatherSettings = UTATWeatherSettings::Get();
   SetGlobalMaterialAndNiagaraParameter<float>(
      weatherSettings.MaterialParameterNames.CameraWetness,
      weatherSettings.NiagaraParameterNames.CameraWetness,
      newCameraWetness);
}

void ATATWeatherPreset::OnOutsidePhysicalSurfaceNearbyBegin_Implementation(UPhysicalMaterial* physicalMaterial, const FVector& worldLocation, float distanceFromCameraToLocation)
{
#if OSE_CHEATS_ENABLED
   ATATWeatherManager* weatherManager = _GetWeatherManager();
   if (weatherManager != nullptr && weatherManager->GetTemporalTraceDebuggingEnabled())
   {
      WeatherHelpers::SetPhysicalSurfaceDebugMessage(physicalMaterial, FString::Printf(TEXT("BEGIN (distance = %.2f)"), distanceFromCameraToLocation), FColor::Green);
   }
#endif
}

void ATATWeatherPreset::OnOutsidePhysicalSurfaceNearbyUpdate_Implementation(UPhysicalMaterial* physicalMaterial, const FVector& worldLocation, float distanceFromCameraToLocation)
{
#if OSE_CHEATS_ENABLED
   ATATWeatherManager* weatherManager = _GetWeatherManager();
   if (weatherManager != nullptr && weatherManager->GetTemporalTraceDebuggingEnabled())
   {
      UE_LOG(LogTATWeatherPreset, Verbose, TEXT("OnOutsidePhysicalSurfaceNearbyUpdate(physicalMaterial='%s', worldLocation=%s, distanceFromCameraToLocation=%.2f"),
         (physicalMaterial ? *physicalMaterial->GetName() : TEXT("NULL")), *worldLocation.ToCompactString(), distanceFromCameraToLocation);
   }
#endif
}

void ATATWeatherPreset::OnOutsidePhysicalSurfaceNearbyEnd_Implementation(UPhysicalMaterial* physicalMaterial, const FVector& worldLocation)
{
#if OSE_CHEATS_ENABLED
   ATATWeatherManager* weatherManager = _GetWeatherManager();
   if (weatherManager != nullptr && weatherManager->GetTemporalTraceDebuggingEnabled())
   {
      WeatherHelpers::SetPhysicalSurfaceDebugMessage(physicalMaterial, TEXT("END"), FColor::Orange);
   }
#endif
}

bool ATATWeatherPreset::GetClosestWorldLocationToPhysicalSurface(UPhysicalMaterial* physicalMaterial, FVector& worldLocation) const
{
   if (physicalMaterial != nullptr)
   {
      const FTATNearbyPhysicalSurface* nearbyPhysicalSurface = _nearbyPhysicalSurfaces.Find(physicalMaterial);
      if (nearbyPhysicalSurface != nullptr && nearbyPhysicalSurface->IsValid)
      {
         worldLocation = nearbyPhysicalSurface->WorldLocation;
         return true;
      }
   }
   worldLocation = FVector::ZeroVector;
   return false;
}

bool ATATWeatherPreset::GetDistanceToNearbyPhysicalSurface(UPhysicalMaterial* physicalMaterial, FVector& cameraWorldLocation, FVector& surfaceWorldLocation, float& distance, float& normalizedDistance) const
{
   if (physicalMaterial != nullptr)
   {
      const FTATNearbyPhysicalSurface* nearbyPhysicalSurface = _nearbyPhysicalSurfaces.Find(physicalMaterial);
      if (nearbyPhysicalSurface != nullptr && nearbyPhysicalSurface->IsValid && GetCameraViewpoint(cameraWorldLocation))
      {
         surfaceWorldLocation = nearbyPhysicalSurface->WorldLocation;
         distance = FVector::Distance(cameraWorldLocation, nearbyPhysicalSurface->WorldLocation);
         normalizedDistance = FMath::Clamp(distance / FMath::Max(1.0f, MaxDistanceToNearbyPhysicalSurface), 0.0f, 1.0f);
         return true;
      }
   }
   cameraWorldLocation = FVector::ZeroVector;
   surfaceWorldLocation = FVector::ZeroVector;
   distance = MaxDistanceToNearbyPhysicalSurface;
   normalizedDistance = 1.0f;
   return false;
}

bool ATATWeatherPreset::FindClosestKnownOutsideWorldLocationToCameraViewpoint(FVector& outdoorWorldLocation) const
{
   ATATWeatherManager* weatherManager = _GetWeatherManager();
   if (weatherManager == nullptr)
   {
      outdoorWorldLocation = FVector::ZeroVector;
      return false;
   }

   // If the camera is outside, then that's the closest location
   if (!weatherManager->IsCameraViewpointInside())
   {
      return weatherManager->GetCameraViewpoint(outdoorWorldLocation);
   }

   if (TOptional<FVector> lastKnownOutdoorWorldLocation = weatherManager->GetLastKnownOutdoorWorldLocation())
   {
      outdoorWorldLocation = *lastKnownOutdoorWorldLocation;
      return true;
   }

   outdoorWorldLocation = FVector::ZeroVector;
   return false;
}

float ATATWeatherPreset::GetTargetNoiseLevel() const
{
   ATATWeatherManager* weatherManager = _GetWeatherManager();
   return (weatherManager != nullptr && weatherManager->IsCameraViewpointInside()) ? NoiseLevelInside : NoiseLevelOutside;
}

void ATATWeatherPreset::_PollNearbyOutsidePhysicalSurfaces()
{
   ATATWeatherManager* weatherManager = _GetWeatherManager();
   if (weatherManager == nullptr)
   {
      return;
   }

   FVector cameraViewpoint;
   if (!weatherManager->GetCameraViewpoint(cameraViewpoint))
   {
      return;
   }

   SCOPE_CYCLE_COUNTER(STAT_WeatherPresetTick_PollPhysSurfaces);

   // Compare a distance with a squared distance and return true if the difference between them is greater than threshold
   auto isDistanceDeltaGreaterThan = [](float oldDistance, float newDistanceSquared, float delta)
   {
      return newDistanceSquared >= FMath::Square(oldDistance + delta) || newDistanceSquared <= FMath::Square(oldDistance - delta);
   };

   const float maxNearbyDistanceSquared = FMath::Square(MaxDistanceToNearbyPhysicalSurface);

   // First, update all nearby physical surfaces to account for current temporal environment trace data
   for (const FTATTemporalEnvironmentTraceEvent& evt : weatherManager->GetTemporalEnvironmentTraceData())
   {
      if (!evt.IsValid() || !evt.OutsideTraceHitPhysicalMaterial)
      {
         continue;
      }

      FTATNearbyPhysicalSurface* nearbyPhysicalSurface = TrackAllPhysicalSurfaceTypes
         ? &_nearbyPhysicalSurfaces.FindOrAdd(evt.OutsideTraceHitPhysicalMaterial)
         : _nearbyPhysicalSurfaces.Find(evt.OutsideTraceHitPhysicalMaterial);

      if (nearbyPhysicalSurface == nullptr)
      {
         continue;
      }

      const float curDistToSurfacePointSquared = FVector::DistSquared(cameraViewpoint, evt.WorldLocation);
      if (curDistToSurfacePointSquared > maxNearbyDistanceSquared)
      {
         continue;
      }

      if (nearbyPhysicalSurface->IsValid)
      {
         // We only store the closest point, so we don't need to do anything here unless this new point is closer than the one we've got
         if (curDistToSurfacePointSquared < FVector::DistSquared(cameraViewpoint, nearbyPhysicalSurface->WorldLocation))
         {
            nearbyPhysicalSurface->WorldLocation = evt.WorldLocation;
         }
      }
      else
      {
         nearbyPhysicalSurface->IsValid = true;
         nearbyPhysicalSurface->WorldLocation = evt.WorldLocation;
         nearbyPhysicalSurface->DistanceAtLastSurfaceEvent = FMath::Sqrt(curDistToSurfacePointSquared);
         OnOutsidePhysicalSurfaceNearbyBegin(evt.OutsideTraceHitPhysicalMaterial, evt.WorldLocation, nearbyPhysicalSurface->DistanceAtLastSurfaceEvent);
         INC_DWORD_STAT(STAT_WeatherPreset_NumTrackedPhysicalSurfaces);
      }
   }

   // Second, make any nearby physical surfaces that are no longer "nearby" invalid.
   // Also fire update events for nearby surfaces that have moved relative to the camera location if needed.
   for (auto& pair : _nearbyPhysicalSurfaces)
   {
      if (!pair.Value.IsValid)
      {
         continue;
      }

      const float curDistanceSquared = FVector::DistSquared(cameraViewpoint, pair.Value.WorldLocation);
      if (curDistanceSquared > maxNearbyDistanceSquared)
      {
         OnOutsidePhysicalSurfaceNearbyEnd(pair.Key, pair.Value.WorldLocation);
         DEC_DWORD_STAT(STAT_WeatherPreset_NumTrackedPhysicalSurfaces);
         pair.Value.Reset();
      }
      else if (isDistanceDeltaGreaterThan(pair.Value.DistanceAtLastSurfaceEvent, curDistanceSquared, MinDistanceToNearbyPhysicalSurfaceToTriggerChangeEvent))
      {
         // Fire an update event only if the new distance is over the threshold (to avoid firing continuous events when the player is moving around a small amount)
         pair.Value.DistanceAtLastSurfaceEvent = FMath::Sqrt(curDistanceSquared);
         OnOutsidePhysicalSurfaceNearbyUpdate(pair.Key, pair.Value.WorldLocation, pair.Value.DistanceAtLastSurfaceEvent);
      }
   }
}

ATATWeatherManager* ATATWeatherPreset::_GetWeatherManager() const
{
   if (ATATWeatherManager* mgr = GetOwner<ATATWeatherManager>())
   {
      return mgr;
   }

   UWorld* world = GetWorld();

#if WITH_EDITOR
   if (world == nullptr && GEditor != nullptr && !GEditor->IsPlayingSessionInEditor())
   {
      world = GEditor->GetWorld();
   }
#endif

   if (world != nullptr)
   {
      if (ATATWorldSettings* worldSettings = Cast<ATATWorldSettings>(world->GetWorldSettings()))
      {
         return worldSettings->WeatherManager;
      }
   }

   return nullptr;
}

UMaterialParameterCollection* ATATWeatherPreset::_GetMaterialParameterCollection() const
{
   if (_cachedMaterialParameterCollection == nullptr)
   {
      const_cast<ATATWeatherPreset*>(this)->_cachedMaterialParameterCollection = UTATWeatherSettings::Get().MaterialParameterCollection.LoadSynchronous();
   }
   return _cachedMaterialParameterCollection;
}

UNiagaraParameterCollection* ATATWeatherPreset::_GetNiagaraParameterCollection() const
{
   if (_cachedNiagaraParameterCollection == nullptr)
   {
      const_cast<ATATWeatherPreset*>(this)->_cachedNiagaraParameterCollection = UTATWeatherSettings::Get().NiagaraParameterCollection.LoadSynchronous();
   }
   return _cachedNiagaraParameterCollection;
}

void ATATWeatherPreset::_CalcCurrentWindDirectionAndStrength(FVector& outCurrentDirection, float& outCurrentStrength) const
{
   if (IsWindEnabled())
   {
      const float currentWorldTime = UTATWeatherUtilities::GetWeatherTimeSeconds(this);
      outCurrentDirection = CalcWindDirection(currentWorldTime);
      outCurrentStrength = CalcWindStrength(currentWorldTime);
   }
   else
   {
      outCurrentDirection = FVector::ForwardVector;
      outCurrentStrength = 0.0f;
   }
}

void ATATWeatherPreset::_TrySetParticleComponentActivated(UNiagaraComponent* comp, bool activated, const TSoftObjectPtr<UNiagaraSystem>& defaultAsset, const TCHAR* particleType)
{
   if (comp == nullptr)
   {
      return;
   }

   if (!activated)
   {
      comp->DeactivateImmediate();
      return;
   }

   static constexpr bool resetParticleSystem = true;

   if (comp->GetAsset() == nullptr)
   {
      if (defaultAsset.IsNull())
      {
         UE_LOG(LogTATWeatherPreset, Error, TEXT("[WeatherPreset %s] Failed to enable %s particles: the component has no particle asset assigned"),
            *GetName(), particleType);
      }
      else
      {
         // We don't have an assigned particle system asset, but we do have a default one. Async-load that and activate it.
         UAssetManager::GetStreamableManager().RequestAsyncLoad(defaultAsset.ToSoftObjectPath(), [weakComp = MakeWeakObjectPtr(comp), defaultAsset]()
         {
            if (UNiagaraComponent* comp = weakComp.Get())
            {
               comp->SetAsset(defaultAsset.Get());
               comp->Activate(resetParticleSystem);
            }
         });
      }
      return;
   }

   comp->Activate(resetParticleSystem);
}

#undef LOCTEXT_NAMESPACE
