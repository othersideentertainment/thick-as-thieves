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

#ifndef __AK_PLATFORM_CONTEXT_PS5_H__
#define __AK_PLATFORM_CONTEXT_PS5_H__

#include <AK/SoundEngine/Common/IAkPlatformContext.h>
#include <audio_out2.h>

namespace AK
{
	/// Context specific to the PS5 port of Wwise SDK.
	class IAkPS5Context : public IAkPlatformContext
	{
	public:
		/// Returns the current SceAudioOut2ContextHandle being used by the Wwise SoundEngine for main output.
		/// This should be called only once the SoundEngine has been successfully initialized, otherwise
		/// the function will return an invalid value (SCE_AUDIO_OUT2_CONTEXT_HANDLE_INVALID). A sink
		/// does not necessarily have had to be created first to utilize this. Note that if a value besides
		/// SCE_AUDIO_OUT2_CONTEXT_HANDLE_INVALID is returned, that value will persist for as long as the sound
		/// engine is initialized.
		///
		/// \return the current sceAudioOut2 context handle or SCE_AUDIO_OUT2_CONTEXT_HANDLE_INVALID.
		virtual SceAudioOut2ContextHandle GetAudioOutContextHandle() = 0;
	};
}

#endif // __AK_PLATFORM_CONTEXT_PS5_H__
