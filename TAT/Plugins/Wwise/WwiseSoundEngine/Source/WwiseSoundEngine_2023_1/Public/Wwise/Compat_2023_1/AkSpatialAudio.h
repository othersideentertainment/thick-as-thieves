/*******************************************************************************
The content of this file includes portions of the proprietary AUDIOKINETIC Wwise
Technology released in source code form as part of the game integration package.
The content of this file may not be used without valid licenses to the
AUDIOKINETIC Wwise Technology.
Note that the use of the game engine is subject to the Unreal(R) Engine End User
License Agreement at https://www.unrealengine.com/en-US/eula/unreal
 
License Usage
 
Licensees holding valid licenses to the AUDIOKINETIC Wwise Technology may use
this file in accordance with the end user license agreement provided with the
software or, alternatively, in accordance with the terms contained
in a written agreement between you and Audiokinetic Inc.
Copyright (c) 2025 Audiokinetic Inc.
*******************************************************************************/

#pragma once

#include "AkInclude.h"

/// Determines the operation used to determine the transmission loss value for a path.
/// A ray is cast from listener to emitter, and this operation is applied to combine the current surface with the previous.
enum AkTransmissionOperation : AkUInt8
{
	AkTransmissionOperation_Add,				///< Transmission loss of each hit surface is summed until it reaches 100%. If a geometry instance is solid, the max of each hit side is added for that instance.
	AkTransmissionOperation_Multiply,			///< The inverse of transmission loss (1 - TL) is multiplied in succession, and the result inverted. With each hit surface, the effect of additional transmission loss is reduced. The total loss will approach but never reach 100% unless a surface with 100% loss is found.
	AkTransmissionOperation_Max,				///< The highest transmission loss of all hit surfaces is used.
	AkTransmissionOperation_Default = AkTransmissionOperation_Max,
};