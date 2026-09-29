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

#include "Wwise/API_2022_1/Platforms/SwitchAPI_2022_1.h"
#include "Wwise/Stats/SoundEngine.h"

#if defined(PLATFORM_SWITCH) && PLATFORM_SWITCH

#if !defined(__has_include) || __has_include("Generated/AkSwitchPlugins.h")
#include "Generated/AkSwitchPlugins.h"
#endif

#if AK_SUPPORT_OPUS
#include <AK/Plugin/AkOpusNXFactory.h>
#endif

#endif
