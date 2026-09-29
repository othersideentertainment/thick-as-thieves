// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "Camera/PlayerCameraManager.h"
#include "OSEPlayerCameraManager.generated.h"


//--------------------------------------------------------------------------------------------------
/// Custom version of the player camera manager. Provides support for various camera utilities,
/// interfaces and requirements 
//--------------------------------------------------------------------------------------------------

UCLASS()
class OSECORE_API AOSEPlayerCameraManager : public APlayerCameraManager
{
   GENERATED_BODY()

public:

   /// Sets default values for the camera manager
   AOSEPlayerCameraManager();
};
