// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

#include "TATWeatherPresetDedicatedServer.generated.h"

class ATATWeatherPreset;
class UDirectionalLightComponent;
class UOSELightEmittingComponent;

/// A dummy weather preset used only on dedicated servers which copies
/// necessary information from the current preset to components it manages.
UCLASS(NotBlueprintable, NotPlaceable)
class TAT_API ATATWeatherPresetDedicatedServer : public AActor
{
   GENERATED_BODY()

public:
   ATATWeatherPresetDedicatedServer();

   // from AActor
   virtual void PostInitializeComponents() override;

   UPROPERTY()
   TObjectPtr<UDirectionalLightComponent> PrimaryDirectionalLightComponent;

   UPROPERTY()
   TObjectPtr<UOSELightEmittingComponent> PrimaryDirectionalLightEmitterComponent;
   
};
