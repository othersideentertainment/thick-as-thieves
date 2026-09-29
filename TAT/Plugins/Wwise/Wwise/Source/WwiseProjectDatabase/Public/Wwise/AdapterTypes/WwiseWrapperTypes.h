/*******************************************************************************
The content of this file includes portions of the proprietary AUDIOKINETIC Wwise
Technology released in source code form as part of the game integration package.
The content of this file may not be used without valid licenses to the
AUDIOKINETIC Wwise Technology.
Note that the use of the game engine is subject to the Unreal(R) Engine End User
License Agreement at https://www.unrealengine.com/en-US/eula/Unreal
 
License Usage
 
Licensees holding valid licenses to the AUDIOKINETIC Wwise Technology may use
this file in accordance with the end user license agreement provided with the
software or, alternatively, in accordance with the terms contained
in a written agreement between you and Audiokinetic Inc.
Copyright (c) 2025 Audiokinetic Inc.
*******************************************************************************/

#pragma once

#if defined(WWISE_DB_STANDARD_TYPES)
#include "Standard/WwiseStandardTypes.h"
#elif defined(WWISE_DB_UNREAL_TYPES)
#include "Unreal/WwiseUnrealTypes.h"
#else
#define WWISE_DB_STANDARD_TYPES
#warning No Adapter Types Defined, falling back on Wwise Standard Types
#include "Standard/WwiseStandardTypes.h"
#endif
