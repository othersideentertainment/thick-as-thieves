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

#include "Wwise/API_2023_1/Platforms/WinGDKAPI_2023_1.h"
#include "Wwise/Stats/SoundEngine.h"

#if defined(PLATFORM_WINGDK) && PLATFORM_WINGDK
AkUInt32 FWwisePlatformAPI_2023_1_WinGDK::GetDeviceID(IMMDevice* in_pDevice)
{
	SCOPE_CYCLE_COUNTER(STAT_WwiseSoundEngineAPI);
	return AK::GetDeviceID(in_pDevice);
}

AkUInt32 FWwisePlatformAPI_2023_1_WinGDK::GetDeviceIDFromName(wchar_t* in_szToken)
{
	SCOPE_CYCLE_COUNTER(STAT_WwiseSoundEngineAPI);
	return AK::GetDeviceIDFromName(in_szToken);
}

const wchar_t* FWwisePlatformAPI_2023_1_WinGDK::GetWindowsDeviceName(AkInt32 index, AkUInt32& out_uDeviceID,
	AkAudioDeviceState uDeviceStateMask)
{
	SCOPE_CYCLE_COUNTER(STAT_WwiseSoundEngineAPI);
	return AK::GetWindowsDeviceName(index, out_uDeviceID, uDeviceStateMask);
}

AkUInt32 FWwisePlatformAPI_2023_1_WinGDK::GetWindowsDeviceCount(AkAudioDeviceState uDeviceStateMask)
{
	SCOPE_CYCLE_COUNTER(STAT_WwiseSoundEngineAPI);
	return AK::GetWindowsDeviceCount(uDeviceStateMask);
}

bool FWwisePlatformAPI_2023_1_WinGDK::GetWindowsDevice(AkInt32 in_index, AkUInt32& out_uDeviceID,
	IMMDevice** out_ppDevice, AkAudioDeviceState uDeviceStateMask)
{
	SCOPE_CYCLE_COUNTER(STAT_WwiseSoundEngineAPI);
	return AK::GetWindowsDevice(in_index, out_uDeviceID, out_ppDevice, uDeviceStateMask);
}

//AkDeviceID FWwisePlatformAPI_2023_1_WinGDK::GetDeviceIDFromGamepad(Windows::Gaming::Input::Gamepad^ rGamepad)
//{
//	SCOPE_CYCLE_COUNTER(STAT_WwiseSoundEngineAPI);
//	return AK::SoundEngine::GetWindowsDevice(rGamepad);
//}
#endif
