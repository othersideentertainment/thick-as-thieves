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

#if defined(PLATFORM_PS5) && PLATFORM_PS5

#include "AkInclude.h"
#include "AkPS5InitializationSettings.h"

#define TCHAR_TO_AK(Text) (const ANSICHAR*)(TCHAR_TO_ANSI(Text))
using UAkInitializationSettings = UAkPS5InitializationSettings;

struct FAkPS5Platform
{
	static const UAkInitializationSettings* GetInitializationSettings()
	{
		return GetDefault<UAkInitializationSettings>();
	}

	static const FString GetPlatformBasePath()
	{
		return FString("PS5");
	}

	static FString GetWwiseSoundEnginePluginDirectory();
	static FString GetDSPPluginsDirectory(const FString& PlatformArchitecture);
	static void PreInitialize(const FAkInitializationStructure& InitSettings);
};

using FAkPlatform = FAkPS5Platform;

#endif
