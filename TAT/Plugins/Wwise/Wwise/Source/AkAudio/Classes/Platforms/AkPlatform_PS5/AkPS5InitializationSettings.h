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

#include "AkInclude.h"
#include "InitializationSettings/AkInitializationSettings.h"
#include "InitializationSettings/AkPlatformInitializationSettingsBase.h"

#include "AkPS5InitializationSettings.generated.h"


USTRUCT()
struct FAkPS5AdvancedInitializationSettings : public FAkAdvancedInitializationSettingsWithMultiCoreRendering
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Ak Initialization Settings", meta = (ToolTip = "Use low latency mode for hardware codecs (default is false). If true, decoding jobs are submitted at the beginning of the Wwise update and it will be necessary to wait for the result."))
	bool UseHardwareCodecLowLatencyMode = false;

	UPROPERTY(EditAnywhere, Category = "Ak Initialization Settings", meta = (ToolTip = "Decode all Vorbis sources on PS5's audio co-processor, similar to ATRAC9. Please refer to the PS5-specific section of the Wwise SDK documentation for more information on advantages and limitations of hardware decoders."))
	bool bVorbisHwAcceleration = false;

	UPROPERTY(EditAnywhere, Category = "Ak Initialization Settings", meta = (ToolTip = "Controls synchronization of 3D Audio across the ambisonic mix, and passthrough/objects. When enabled, this increases the delay of the Passthrough and Objects audio mixes by a few milliseconds, in order to be in sync with the main Ambisonic mix. Refer to sceAudioOut2Set3DLatency in the platform documentation for more information."))
	bool bEnable3DAudioSync = true;

	UPROPERTY(EditAnywhere, Category = "Ak Initialization Settings", DisplayName="NumOperationsForHwMixing", meta = (ToolTip = "Used to determine the threshold at which bus-mix operations will be performed in hardware instead of software. Please refer to the PS5-specific section of the Wwise SDK documentation for more information on this tradeoff."))
	uint32 uNumOperationsForHwMixing = 1300;

	UPROPERTY(EditAnywhere, Category = "Ak Initialization Settings", meta = (ToolTip = "If enabled, will constantly output the current context queue level as a plot to Razor CPU, to visualize audio starvation limits."))
	bool bPlotQueueLevel = true;

	UPROPERTY(EditAnywhere, Category = "Ak Initialization Settings", DisplayName="Num AudioOut2 Ports", meta = (ToolTip = "Determines how many AudioOut2 ports the soundengine will be initialized with. One port is required for each non-3D output device being used by Wwise, including Motion."))
	uint32 NumAudioOut2Ports = 16;

	UPROPERTY(EditAnywhere, Category = "Ak Initialization Settings", DisplayName="Num AudioOut2 Object Ports", meta = (ToolTip = "Determines how many AudioOut2 Object ports the soundengine will be initialized with for 3D Audio processing. Setting this to zero will disable all use of 3D Audio. Note that each ambisonic channel and passthrough channel counts as an Object port, so the default value of 128 will allow for 90 Audio Object channels to be used for 3D Audio Processing."))
	uint32 NumAudioOut2ObjectPorts = 128;

	void FillInitializationStructure(FAkInitializationStructure& InitializationStructure) const;
};


UCLASS(config = Game, defaultconfig)
class AKAUDIO_API UAkPS5InitializationSettings : public UAkPlatformInitializationSettingsBase
{
	GENERATED_BODY()

public:
	virtual const TCHAR* GetConfigOverridePlatform() const override
	{
		return TEXT("PS5");
	}

	UAkPS5InitializationSettings(const FObjectInitializer& ObjectInitializer);
	void FillInitializationStructure(FAkInitializationStructure& InitializationStructure) const;

	UPROPERTY(Config, EditAnywhere, Category = "Initialization")
	FAkCommonInitializationSettings CommonSettings;

	UPROPERTY(Config, EditAnywhere, Category = "Initialization")
	FAkCommunicationSettingsWithSystemInitialization CommunicationSettings;

	UPROPERTY(Config, EditAnywhere, Category = "Initialization", AdvancedDisplay)
	FAkPS5AdvancedInitializationSettings AdvancedSettings;
};

UCLASS()
class AKAUDIO_API UAkDPXInitializationSettings : public UAkPS5InitializationSettings
{
	GENERATED_BODY()
};
