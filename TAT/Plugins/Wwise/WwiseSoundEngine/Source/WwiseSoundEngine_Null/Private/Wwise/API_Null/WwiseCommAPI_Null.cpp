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

#include "Wwise/API_Null/WwiseCommAPI_Null.h"
#include "Wwise/Stats/SoundEngine.h"

AKRESULT FWwiseCommAPI_Null::Init(
	const AkCommSettings& in_settings
)
{
	return AK_NotImplemented;
}

AkInt32 FWwiseCommAPI_Null::GetLastError()
{
	return 0;
}

void FWwiseCommAPI_Null::GetDefaultInitSettings(
	AkCommSettings& out_settings
)
{
}

void FWwiseCommAPI_Null::Term()
{
}

AKRESULT FWwiseCommAPI_Null::Reset()
{
	return AK_NotImplemented;
}

const AkCommSettings& FWwiseCommAPI_Null::GetCurrentSettings()
{
	static const AkCommSettings StaticSettings;
	return StaticSettings;
}

AkUInt16 FWwiseCommAPI_Null::GetCommandPort()
{
	return 0;
}
