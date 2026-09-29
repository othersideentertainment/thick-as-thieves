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
/// Main Sound Engine interface for the Gaming.Xbox-specific platform.
/// Currently, Xbox One and Xbox Series X share similar settings and interface via the GDK

#pragma once

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

struct IMMDevice;
struct IGameInputDevice;

/// Platform specific initialization settings
/// \sa AK::SoundEngine::Init
/// \sa AK::SoundEngine::GetDefaultPlatformInitSettings
struct AkPlatformInitSettings
{
	// Threading model.
	AkThreadProperties  threadLEngine;			///< Lower engine threading properties
	AkThreadProperties  threadOutputMgr;		///< Ouput thread threading properties
	AkThreadProperties  threadBankManager;		///< Bank manager threading properties (its default priority is AK_THREAD_PRIORITY_NORMAL)
	AkThreadProperties  threadMonitor;			///< Monitor threading properties (its default priority is AK_THREAD_PRIORITY_ABOVENORMAL). This parameter is not used in Release build.

	// Voices.
	AkUInt16            uNumRefillsInVoice;		///< Number of refill buffers in voice buffer. 2 == double-buffered, defaults to 4.
	bool				bHwCodecLowLatencyMode; ///< Use low latency mode for hardware Opus decoding (default is false).  If true, decoding jobs are submitted at the beginning of the Wwise update and it will be necessary to wait for the result.
#ifdef AK_XBOXSERIESX
	AkUInt16			uMaxOpusVoices;			///< Maximum number of hardware-accelerated Opus voices used at run-time. Default is 320 voices, the maximum value. Reduce to save on XAPU memory, or set to zero to use software decoding.	
#endif
	AkUInt32			uMaxSystemAudioObjects; ///< Dictates how many Microsoft Spatial Sound dynamic objects will be reserved by the System sink. Set to 0 to disable the use of System Audio Objects. Default is 256.

#ifdef AK_XBOXSERIESX
	// XDSP configuration for 'AK Convolution' plug-in
	AkUInt32			uMaxXdspStreams;		///< Number of streams to initialize XDSP with. Maximum of 256. Less than 16 will leave XDSP uninitialized. Note that each channel of active convolution processing counts as 1 stream. Lower values will reduce memory use. Streams that can't fit will instead use a software fallback.
	AkUInt32			uMaxXdspAggregateStreamLength;	///< Maximum aggregate length of the impulse responses that can be running simultaneously, in seconds. Maximum of 128. Lower values will reduce memory use. Streams that can't fit will instead use a software fallback.
#endif
};

namespace AK
{
	/// Finds the device ID for particular Audio Endpoint.  
	/// \return A device ID to use with AddSecondaryOutput
	AK_EXTERNAPIFUNC( AkUInt32, GetDeviceID ) (IMMDevice* in_pDevice);

	/// Finds an audio endpoint that matches the token in the device name or device ID and returns and ID compatible with AddSecondaryOutput.  
	/// This is a helper function that searches in the device ID (as returned by IMMDevice->GetId) and IMMXboxDevice->GetPnpId()
	/// If you need to do matching on different conditions, use IMMXboxDeviceEnumerator directly.
	/// \return An ID to use with AddSecondaryOutput.  The ID returned is the device ID as returned by IMMDevice->GetId, hashed by AK::SoundEngine::GetIDFromName()
	AK_EXTERNAPIFUNC( AkUInt32, GetDeviceIDFromName )(wchar_t* in_szToken);

	namespace SoundEngine
	{

		/// Finds the device ID for particular GameInput device.
		/// \return A device ID to use with AddSecondaryOutput
		AK_EXTERNAPIFUNC(AkUInt32, GetGameInputDeviceID) (const IGameInputDevice* in_pGameInputDevice);
	}
}
