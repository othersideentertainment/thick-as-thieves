// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Camera/OSECameraComponent.h"
#include "Camera/OSECameraSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSECameraComponent)


UOSECameraComponent::UOSECameraComponent()
   : Super()
{
   const FOSECameraParams& playerParams = UOSECameraSettings::Get().GetPlayerParams();
   FieldOfView = playerParams.FieldOfView;
}

