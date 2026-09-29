// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "Camera/CameraComponent.h"

#include "OSECameraComponent.generated.h"


//--------------------------------------------------------------------------------------------------
/// Custom version of the camera component. Most camera modifications should be done through
/// modifiers applied to the camera manager.
//--------------------------------------------------------------------------------------------------

UCLASS()
class OSECORE_API UOSECameraComponent : public UCameraComponent
{
   GENERATED_BODY()

public:

   UOSECameraComponent();

};
