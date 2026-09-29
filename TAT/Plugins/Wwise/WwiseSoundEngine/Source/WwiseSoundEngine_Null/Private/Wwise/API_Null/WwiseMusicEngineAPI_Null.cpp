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

#include "Wwise/API_Null/WwiseMusicEngineAPI_Null.h"
#include "Wwise/Stats/SoundEngine.h"

AKRESULT FWwiseMusicEngineAPI_Null::Init(
	AkMusicSettings* in_pSettings
)
{
	return AK_NotImplemented;
}

void FWwiseMusicEngineAPI_Null::GetDefaultInitSettings(
	AkMusicSettings& out_settings
)
{
}

void FWwiseMusicEngineAPI_Null::Term(
)
{
}

AKRESULT FWwiseMusicEngineAPI_Null::GetPlayingSegmentInfo(
	AkPlayingID		in_PlayingID,
	AkSegmentInfo& out_segmentInfo,
	bool			in_bExtrapolate
)
{
	return AK_NotImplemented;
}
