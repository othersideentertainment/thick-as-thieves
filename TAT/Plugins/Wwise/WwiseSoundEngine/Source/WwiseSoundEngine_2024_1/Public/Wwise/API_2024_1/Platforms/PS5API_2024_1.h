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

#include "Wwise/API/Platforms/PS5API.h"

#if defined(PLATFORM_PS5) && PLATFORM_PS5

class FWwisePlatformAPI_2024_1_PS5 : public IWwisePlatformAPI
{
public:
	UE_NONCOPYABLE(FWwisePlatformAPI_2024_1_PS5);
	FWwisePlatformAPI_2024_1_PS5() = default;
	virtual ~FWwisePlatformAPI_2024_1_PS5() override {}

	/// Returns the current SceAudioOut2PortHandle being used by the Wwise SoundEngine for main output.
	/// This should be called only once the SoundEngine has been successfully initialized, otherwise
	/// the function will return an invalid value (SCE_AUDIO_OUT2_PORT_HANDLE_INVALID).
	///
	/// Use <tt>AK::SoundEngine::RegisterAudioDeviceStatusCallback()</tt> to get notified when devices are created/destructed.
	/// 
	/// Note that because this function acquires the global lock in Wwise in order to fetch device state,
	/// it is recommended to call this inside of the AudioDeviceStatusCallback, after a successful AkAudioDeviceEvent_Initialization.
	///
	/// \return the current sceAudioOut2 main output port handle or SCE_AUDIO_OUT2_PORT_HANDLE_INVALID.
	virtual SceAudioOut2PortHandle GetAudioOut2PortHandle(AkOutputDeviceID deviceId = 0) override;

	/// Load the dynamic support library which enables Vorbis HW acceleration. This must be done once before any Vorbis-encoded voice starts to play.
	///
	/// The plug-in DLL must be in the OS-specific library path or in the same location as the executable. If not, set AkInitSettings.szPluginDLLPath.
	/// This function can be called safely before AK::SoundEngine::Init(), but in this case you must provide a valid DLL path.
	/// 
	/// \return
	/// - Ak_Success if successful.
	/// - AK_FileNotFound if the DLL is not found in the OS path or if it has extraneous dependencies not found.
	/// - AK_InsufficientMemory if ran out of resources while loading library.
	/// - AK_NotCompatible if version of the PS5 SDK used to build the specified dynamic library is newer than the system software version.
	/// - AK_InvalidFile if the library loaded does not export the expected symbols.
	/// - AK_Fail if an unexpected error occured.
	virtual AKRESULT LoadVorbisHwAcceleratorLibrary(const AkOSChar* in_DllName, const AkOSChar* in_DllPath = NULL) override;

	/// Using data provided by SceAudioOut2SpeakerInfo, as obtained by sceAudioOut2GetSpeakerInfo,
	/// populates out_pfSpeakerAngles and out_rfHeightAngle with appropriate data to use for AK::SoundEngine::SetSpeakerAngles.
	/// 
	/// If you are using this to set speaker angles manually, ensure that new angles are recognized and set, even
	/// if the user's speaker configuration changes, by periodically checking for differences in SceAudioOut2SpeakerInfo's state.
	///
	/// Note that when AkPlatformInitSettings::bUseAudioOut2SpeakerAngles is enabled, the system calls
	/// AK::SoundEngine::SetSpeakerAngles for system output devices automatically at initialization, and when any change
	/// in the user's speaker configuration is detected.
	virtual void GetAudioOut2SpeakerAngles(AkReal32 out_pfSpeakerAngles[3], AkReal32 &out_rfHeightAngle, const SceAudioOut2SpeakerInfo &in_rInfo) override;
};

using FWwisePlatformAPI = FWwisePlatformAPI_2024_1_PS5;

#endif
