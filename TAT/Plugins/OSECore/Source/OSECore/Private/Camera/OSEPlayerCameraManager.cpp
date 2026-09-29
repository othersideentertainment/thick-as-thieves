// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Camera/OSEPlayerCameraManager.h"
#include "Camera/OSECameraSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEPlayerCameraManager)


// Sets default values
AOSEPlayerCameraManager::AOSEPlayerCameraManager()
   : Super()
{
   const FOSECameraParams& playerParams = UOSECameraSettings::Get().GetPlayerParams();
   ViewPitchMin = playerParams.ViewPitchMin;
   ViewPitchMax = playerParams.ViewPitchMax;
}

