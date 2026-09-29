// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "UObject/Interface.h"

#include "PlayerCameraInterface.generated.h"


//---------------------------------------------------------------------------------------------------
/// Player camera anim interface. Used to add and remove camera animation settings to a stack to
/// apply to the player camera. This interface does not need to be implemented directly on a player.
/// Instead, it can be implemented on an object that a player or other object has.
/// 
/// \see UPlayerCameraAnimList
//---------------------------------------------------------------------------------------------------

UINTERFACE(MinimalAPI, Category = "Camera|OSE", meta = (CannotImplementInterfaceInBlueprint))
class UPlayerCameraAnimInterface : public UInterface
{
   GENERATED_BODY()
};

class OSECORE_API IPlayerCameraAnimInterface
{
   GENERATED_BODY()

public:

};


//---------------------------------------------------------------------------------------------------
/// Player camera interface. This is a separate interface so that the actual anim stack can live
/// elsewhere, perhaps on an object the player has.
/// 
/// \see UPlayerCameraAnimList
//---------------------------------------------------------------------------------------------------

UINTERFACE(MinimalAPI, Category = "Camera|OSE", meta = (CannotImplementInterfaceInBlueprint))
class UPlayerCameraInterface : public UInterface
{
   GENERATED_BODY()
};

class OSECORE_API IPlayerCameraInterface
{
   GENERATED_BODY()

public:

};
