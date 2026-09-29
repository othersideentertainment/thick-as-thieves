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

#include "Platforms/AkPlatformInfo.h"
#include "AkXboxSeriesXPlatformInfo.generated.h"

UCLASS()
class UAkXboxSeriesXPlatformInfo : public UAkPlatformInfo
{
	GENERATED_BODY()

public:
	UAkXboxSeriesXPlatformInfo()
	{
		WwisePlatform = "XboxSeriesX";

#ifdef AK_XBOXSERIESX_VS_VERSION
		Architecture = "XboxSeriesX_" AK_XBOXSERIESX_VS_VERSION;
#else
		Architecture = "XboxSeriesX_vc160";
#endif

		LibraryFileNameFormat = "{0}.dll";
		DebugFileNameFormat = "{0}.pdb";
#if WITH_EDITORONLY_DATA
		UAkPlatformInfo::UnrealNameToPlatformInfo.Add("XSX", this);
		UAkPlatformInfo::UnrealNameToPlatformInfo.Add("XboxSeriesX", this);
#endif
	}
};

