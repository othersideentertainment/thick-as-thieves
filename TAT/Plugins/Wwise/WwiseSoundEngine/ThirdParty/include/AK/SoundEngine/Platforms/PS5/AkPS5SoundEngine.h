/*******************************************************************************
The content of this file includes portions of the AUDIOKINETIC Wwise Technology
released in source code form as part of the SDK installer package.

Commercial License Usage

Licensees holding valid commercial licenses to the AUDIOKINETIC Wwise Technology
may use this file in accordance with the end user license agreement provided 
with the software or, alternatively, in accordance with the terms contained in a
written agreement between you and Audiokinetic Inc.

Apache License Usage

Alternatively, this file may be used under the Apache License, Version 2.0 (the 
"Apache License"); you may not use this file except in compliance with the 
Apache License. You may obtain a copy of the Apache License at 
http://www.apache.org/licenses/LICENSE-2.0.

Unless required by applicable law or agreed to in writing, software distributed
under the Apache License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
OR CONDITIONS OF ANY KIND, either express or implied. See the Apache License for
the specific language governing permissions and limitations under the License.

  Copyright (c) 2025 Audiokinetic Inc.
*******************************************************************************/

/// \file 
/// Main Sound Engine interface, PS5 specific.

#pragma once

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <audio_out2.h>

/// Platform specific initialization settings
/// \sa AK::SoundEngine::Init
/// \sa AK::SoundEngine::GetDefaultPlatformInitSettings
struct AkPlatformInitSettings
{
	// Threading model.
	AkThreadProperties  threadLEngine;           ///< Lower engine threading properties
	AkThreadProperties  threadOutputMgr;         ///< Ouput thread threading properties
	AkThreadProperties  threadBankManager;       ///< Bank manager threading properties (its default priority is AK_THREAD_PRIORITY_NORMAL)
	AkThreadProperties  threadMonitor;           ///< Monitor threading properties (its default priority is AK_THREAD_PRIORITY_ABOVENORMAL). This parameter is not used in Release build.
	AkThreadProperties  threadAcmQueue;          ///< Acm Job Queue threading properties (its default priority is AK_THREAD_PRIORITY_ABOVENORMAL)
	AkThreadProperties  threadAudioOut;          ///< AudioOut threading properties (its default priority is AK_THREAD_PRIORITY_ABOVENORMAL)
	AkThreadProperties  threadQueueLevel;        ///< Plot Queue Level threading properties (see bPlotQueueLevel; this is not used in Release builds) 

	// Voices.
	AkUInt16            uNumRefillsInVoice;      ///< The queueDepth of the audioOut2Context. May need to be increased if audio starvation occurs, at the cost of greater audio latency. 2 == double-buffered, defaults to 4.	

	// Hardware decoding.
	bool                bHwCodecLowLatencyMode;  ///< Use low latency mode for hardware codecs such as ATRAC9.  If true, decoding jobs are submitted at the beginning of the Wwise update and it will be necessary to wait for the result. Defaults to true.
	AkUInt32            uHwExtraBuffering;       ///< How many frames of audio to decode in advance, beyond the minimum required for the next frame. Ranges from 0 (the default) to 4. Higher values may save total CPU usage, but will lower the number of hardware voices that can be decoded in a single frame without running into voice starvation issues.
	bool                bVorbisHwAcceleration;   ///< Decode Vorbis sources on PS5's audio co-processor, similar to ATRAC9. Requires loading a separate PRX library to work properly. See AK::LoadVorbisHwAcceleratorLibrary.

	// AudioOut2 setup
	AkUInt32            uNumAudioOut2Ports;         ///< The number of ports to initialize the audioOut2Context with. May need to be increased if using many sinks
	AkUInt32            uNumAudioOut2ObjectPorts;   ///< The number of object ports to initialize the audioOut2Context with. Will need to be increased depending on sceAudio3D configuration
	bool                bEnable3DAudioSync;         ///< Controls synchronization of 3D Audio across the ambisonic mix, and passthrough/objects. When enabled, this increases the delay of the Passthrough and Objects audio mixes by a few milliseconds, in order to be in sync with the main Ambisonic mix. Refer to sceAudioOut2Set3DLatency in the platform documentation for more information.
	bool                bUseAudioOut2SpeakerAngles; ///< When enabled, the system output devices automatically call AK::SoundEngine::SetSpeakerAngles, using speaker angles configured by users in the System Software's Sound settings menu. If it is preferable to set speaker angles manually, refer to AK::GetAudioOut2SpeakerAngles.

	// hardware-assisted audio rendering.
	AkUInt32            uNumOperationsForHwMixing; ///< Used to determine the threshold at which bus-mix operations will be performed in hardware instead of software. For example, if 25 input channels need to be mixed to a 5th-order-ambisonics (36-channel) speaker bed (generating 25*36=900 operations) the mix will be performed in hardware if this value is less than 900, but performed in software if this value is greater than or equal to 900.

	// queue level debugging
	bool                bPlotQueueLevel; ///< If enabled, will constantly output the current context queue level as a plot to Razor CPU, to visualize audio starvation limits. A Named Sync for sound engine ticks will also be added.
};

namespace AK
{
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
	extern SceAudioOut2PortHandle GetAudioOut2PortHandle(AkOutputDeviceID deviceId = 0);

	/// \cond !(Web)
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
	extern AKRESULT LoadVorbisHwAcceleratorLibrary(const AkOSChar* in_DllName, const AkOSChar* in_DllPath = NULL);
	/// \endcond

	/// Using data provided by SceAudioOut2SpeakerInfo, as obtained by sceAudioOut2GetSpeakerInfo,
	/// populates out_pfSpeakerAngles and out_rfHeightAngle with appropriate data to use for AK::SoundEngine::SetSpeakerAngles.
	/// 
	/// If you are using this to set speaker angles manually, ensure that new angles are recognized and set, even
	/// if the user's speaker configuration changes, by periodically checking for differences in SceAudioOut2SpeakerInfo's state.
	///
	/// Note that when AkPlatformInitSettings::bUseAudioOut2SpeakerAngles is enabled, the system calls
	/// AK::SoundEngine::SetSpeakerAngles for system output devices automatically at initialization, and when any change
	/// in the user's speaker configuration is detected.
	extern void GetAudioOut2SpeakerAngles(AkReal32 out_pfSpeakerAngles[3], AkReal32 &out_rfHeightAngle, const SceAudioOut2SpeakerInfo &in_rInfo);

};
