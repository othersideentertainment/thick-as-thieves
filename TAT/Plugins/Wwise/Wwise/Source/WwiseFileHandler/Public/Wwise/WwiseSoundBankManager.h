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

#include "Wwise/WwiseFileHandlerModule.h"
#include "Wwise/WwiseStreamableFileHandler.h"

struct FWwiseSoundBankCookedData;

typedef TSharedPtr<IWwiseSoundBankManager> IWwiseSoundBankManagerPtr;

class IWwiseSoundBankManager : public IWwiseStreamableFileHandler
{
public:
	inline static IWwiseSoundBankManagerPtr Get()
	{
		if (auto* Module = IWwiseFileHandlerModule::GetModule())
		{
			return Module->GetSoundBankManager();
		}
		return nullptr;
	}

	using FLoadSoundBankCallback = TUniqueFunction<void(bool bSuccess)>;
	using FUnloadSoundBankCallback = TUniqueFunction<void()>;

	virtual void LoadSoundBank(const FWwiseSoundBankCookedData& InSoundBankCookedData, FLoadSoundBankCallback&& InCallback) = 0;
	virtual void UnloadSoundBank(const FWwiseSoundBankCookedData& InSoundBankCookedData, FUnloadSoundBankCallback&& InCallback) = 0;
	virtual void SetGranularity(uint32 Uint32) = 0;
};
