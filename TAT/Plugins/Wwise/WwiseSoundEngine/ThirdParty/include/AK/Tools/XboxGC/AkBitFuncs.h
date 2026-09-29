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

#pragma once

#include <AK/SoundEngine/Common/AkTypes.h>
#include <intrin.h>

#define AK_BIT_SCAN_INSTRUCTIONS // mark bitscan instructions as defined to avoid duplicate definitions

// On Xbox we can guarantee support of intrinsics like popcnt and tzcnt, so we can utilize those
namespace AKPLATFORM
{
	// AkPopCount64  returns how many set bits there are in the provided value
	// e.g. 0b0000`1111`0000`0011 would return 6
	AkForceInline AkUInt32 AkPopCount64(AkUInt64 in_bits)
	{
		return (AkUInt32)__popcnt64(in_bits);
	}

	// AkPopCount returns how many set bits there are in the provided value
	// e.g. 0b0000`1111`0000`0011 would return 6
	AkForceInline AkUInt32 AkPopCount(AkUInt32 in_bits)
	{
		return (AkUInt32)__popcnt(in_bits);
	}

	// AkBitScanForward returns how many 0s there are until the least-significant-bit is set
	// (or the length of the param if the value is zero)
	// e.g. 0b0000`0000`0001`0000 would return 4
	AkForceInline AkUInt32 AkBitScanForward64(AkUInt64 in_bits)
	{
		return (AkUInt32)_tzcnt_u64(in_bits);
	}

	AkForceInline AkUInt32 AkBitScanForward(AkUInt32 in_bits)
	{
		return (AkUInt32)_tzcnt_u32(in_bits);
	}

	// AkBitScanReverse returns how many 0s there are after the most-significant-bit is set
	// (or the length of the param if the value is zero)
	// e.g. 0b0000`0000`0001`0000 would return 11
	AkForceInline AkUInt32 AkBitScanReverse64(AkUInt64 in_bits)
	{
		return (AkUInt32)_lzcnt_u64(in_bits);
	}

	AkForceInline AkUInt32 AkBitScanReverse(AkUInt32 in_bits)
	{
		return (AkUInt32)_lzcnt_u32(in_bits);
	}
}