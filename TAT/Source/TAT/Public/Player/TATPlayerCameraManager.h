// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE5
#include "CoreMinimal.h"
#include "Camera/OSEPlayerCameraManager.h"

#include "TATPlayerCameraManager.generated.h"

class UMaterialInstanceDynamic;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDynamicMaterialReady);

/**
 * 
 */
UCLASS()
class TAT_API ATATPlayerCameraManager : public AOSEPlayerCameraManager
{
   GENERATED_BODY()
public:

   ATATPlayerCameraManager();
   virtual void BeginPlay();
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason);
   virtual void UpdateCamera(float deltaTime) override;
};
