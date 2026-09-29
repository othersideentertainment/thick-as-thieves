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

#include "Wwise/API_2022_1/Platforms/PS5API_2022_1.h"
#include "Wwise/Stats/SoundEngine.h"

#if defined(PLATFORM_PS5) && PLATFORM_PS5
SceAudioOut2PortHandle FWwisePlatformAPI_2022_1_PS5::GetAudioOut2PortHandle(AkOutputDeviceID deviceId)
{
	SCOPE_CYCLE_COUNTER(STAT_WwiseSoundEngineAPI);
	return AK::GetAudioOut2PortHandle(deviceId);
}
AKRESULT FWwisePlatformAPI_2022_1_PS5::LoadVorbisHwAcceleratorLibrary(const AkOSChar* in_DllName, const AkOSChar* in_DllPath)
{
	SCOPE_CYCLE_COUNTER(STAT_WwiseSoundEngineAPI);
	return AK::LoadVorbisHwAcceleratorLibrary(in_DllName, in_DllPath);
}
void FWwisePlatformAPI_2022_1_PS5::GetAudioOut2SpeakerAngles(AkReal32 out_pfSpeakerAngles[3], AkReal32 &out_rfHeightAngle, const SceAudioOut2SpeakerInfo &in_rInfo)
{
	SCOPE_CYCLE_COUNTER(STAT_WwiseSoundEngineAPI);
	out_pfSpeakerAngles[0] = 45;
	out_pfSpeakerAngles[1] = 90;
	out_pfSpeakerAngles[2] = 135.f;
	out_rfHeightAngle = 54.7f;
}
#endif
