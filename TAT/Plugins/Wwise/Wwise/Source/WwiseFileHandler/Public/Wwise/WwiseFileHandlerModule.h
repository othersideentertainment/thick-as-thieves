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

#include "Modules/ModuleManager.h"
#include "Misc/ConfigCacheIni.h"

class FWwiseFileCache;
class FWwiseIOHook;
class IWwiseSoundBankManager;
class IWwiseExternalSourceManager;
class IWwiseMediaManager;
struct FWwiseExecutionQueue;

typedef TSharedPtr<IWwiseExternalSourceManager> IWwiseExternalSourceManagerPtr;
typedef TSharedPtr<IWwiseMediaManager> IWwiseMediaManagerPtr;
typedef TSharedPtr<IWwiseSoundBankManager> IWwiseSoundBankManagerPtr;

class IWwiseFileHandlerModule : public IModuleInterface
{
public:
	static FName GetModuleName()
	{
		static const FName ModuleName = GetModuleNameFromConfig();
		return ModuleName;
	}

	/**
	 * Checks to see if this module is loaded and ready.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static bool IsAvailable()
	{
		static bool bModuleAvailable = false;
		if (LIKELY(!IsEngineExitRequested()) && LIKELY(bModuleAvailable))
		{
			return true;
		}
		bModuleAvailable = FModuleManager::Get().IsModuleLoaded(GetModuleName());
		return bModuleAvailable;
	}

	static IWwiseFileHandlerModule* GetModule()
	{
		static IWwiseFileHandlerModule* Module = nullptr;
		if (LIKELY(!IsEngineExitRequested()) && LIKELY(Module))
		{
			return Module;
		}
		
		const auto ModuleName = GetModuleName();
		if (ModuleName.IsNone())
		{
			return nullptr;
		}

		FModuleManager& ModuleManager = FModuleManager::Get();
		Module = ModuleManager.GetModulePtr<IWwiseFileHandlerModule>(ModuleName);
		if (UNLIKELY(!Module))
		{
			if (UNLIKELY(IsEngineExitRequested()))
			{
				UE_LOG(LogLoad, Log, TEXT("Skipping reloading missing WwiseFileHandler module: Exiting."));
			}
			else if (UNLIKELY(!IsInGameThread()))
			{
				UE_LOG(LogLoad, Warning, TEXT("Skipping loading missing WwiseFileHandler module: Not in game thread"));
			}
			else
			{
				UE_LOG(LogLoad, Log, TEXT("Loading WwiseFileHandler module: %s"), *ModuleName.GetPlainNameString());
				Module = ModuleManager.LoadModulePtr<IWwiseFileHandlerModule>(ModuleName);
				if (UNLIKELY(!Module))
				{
					UE_LOG(LogLoad, Fatal, TEXT("Could not load WwiseFileHandler module: %s not found"), *ModuleName.GetPlainNameString());
				}
			}
		}

		return Module;
	}

	virtual IWwiseSoundBankManagerPtr GetSoundBankManager() { return nullptr; }
	virtual IWwiseExternalSourceManagerPtr GetExternalSourceManager() { return nullptr; }
	virtual IWwiseMediaManagerPtr GetMediaManager() { return nullptr; }
	virtual FWwiseFileCache* GetFileCache() { return nullptr; }
	virtual FWwiseExecutionQueue* GetBankExecutionQueue() { return nullptr; } 
	virtual FWwiseIOHook* InstantiateIOHook() { return nullptr; }
	virtual IWwiseSoundBankManagerPtr InstantiateSoundBankManager() { return nullptr; }
	virtual IWwiseExternalSourceManagerPtr InstantiateExternalSourceManager() { return nullptr; }
	virtual IWwiseMediaManagerPtr InstantiateMediaManager() { return nullptr; }
	virtual FWwiseFileCache* InstantiateFileCache() { return nullptr; }
	virtual FWwiseExecutionQueue* InstantiateBankExecutionQueue() { return nullptr; } 

private:
	static inline FName GetModuleNameFromConfig()
	{
		FString ModuleName = TEXT("WwiseFileHandler");
		GConfig->GetString(TEXT("Audio"), TEXT("WwiseFileHandlerModuleName"), ModuleName, GEngineIni);
		return FName(ModuleName);
	}
};
