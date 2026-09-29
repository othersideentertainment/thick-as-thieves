// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Environment/TATWeatherPresetDedicatedServer.h"

// tat
#include "Developer/TATWeatherSettings.h"
#include "Environment/TATWeatherPreset.h"

// ose
#include "OSELightEmittingComponent.h"

// ue
#include "Components/DirectionalLightComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWeatherPresetDedicatedServer)

DEFINE_LOG_CATEGORY_STATIC(LogTATWeatherPresetDedicatedServer, Log, All);

ATATWeatherPresetDedicatedServer::ATATWeatherPresetDedicatedServer()
{
   RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("WeatherPresetRootComponent"));
   RootComponent->SetMobility(EComponentMobility::Static);

   PrimaryDirectionalLightComponent = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("PrimaryDirectionalLightComponent"));
   PrimaryDirectionalLightComponent->SetIntensity(0.0f);
   PrimaryDirectionalLightComponent->SetupAttachment(RootComponent);

   PrimaryDirectionalLightEmitterComponent = CreateDefaultSubobject<UOSELightEmittingComponent>(TEXT("PrimaryDirectionalLightEmitterComponent"));
   PrimaryDirectionalLightEmitterComponent->_lightComponent = PrimaryDirectionalLightComponent;
}

void ATATWeatherPresetDedicatedServer::PostInitializeComponents()
{
   Super::PostInitializeComponents();

   check(GetNetMode() == NM_DedicatedServer);

   TSoftClassPtr<ATATWeatherPreset> presetClassSoft = UTATWeatherSettings::GetCurrentWeatherPresetClass(this);
   if (presetClassSoft.IsNull())
   {
      UE_LOG(LogTATWeatherPresetDedicatedServer, Error, 
         TEXT("Found null class for weather preset, failed to load dedicated server weather preset!"));
      return;
   }

   // Match the synchronous loading of the preset from ATATWeatherManager::SetWeatherPresetType.
   const ATATWeatherPreset* presetCDO = GetDefault<ATATWeatherPreset>(presetClassSoft.LoadSynchronous());
   check(presetCDO != nullptr);

   const UDirectionalLightComponent* lightCDO = presetCDO->PrimaryDirectionalLightComponent;
   check(lightCDO != nullptr);

   // There are other fields on the light component that could be copied over but these
   // two are the only data that is needed on the server for the light detection system.
   PrimaryDirectionalLightComponent->SetRelativeTransform(lightCDO->GetRelativeTransform());
   PrimaryDirectionalLightComponent->SetIntensity(lightCDO->Intensity);
}
