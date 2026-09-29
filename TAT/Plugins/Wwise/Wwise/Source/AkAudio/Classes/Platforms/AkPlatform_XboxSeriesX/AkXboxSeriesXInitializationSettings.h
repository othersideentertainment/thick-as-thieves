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

#include "AkXboxSeriesXInitializationSettings.generated.h"


USTRUCT()
struct FAkXSXAdvancedInitializationSettings : public FAkAdvancedInitializationSettingsWithMultiCoreRendering
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Ak Initialization Settings", meta = (ToolTip = "Use low latency mode for hardware decoding (default is false). If true, decoding jobs are submitted at the beginning of the Wwise update and it will be necessary to wait for the result."))
	bool UseHardwareCodecLowLatencyMode = false;

	UPROPERTY(EditAnywhere, Category = "Ak Initialization Settings", meta = (ToolTip = "Maximum number of hardware-accelerated Opus voices used at run-time. Default is 320 voices. Reduce to save APU memory, or set to zero to use software decoding."))
	uint16 MaximumNumberOfOpusVoices = 320;

	UPROPERTY(EditAnywhere, Category = "Ak Initialization Settings", meta = (ToolTip = "Dictates how many Microsoft Spatial Sound dynamic objects will be reserved by the System sink. Set to 0 to disable the use of System Audio Objects."))
	uint32 uMaxSystemAudioObjects = 256;

	UPROPERTY(EditAnywhere, Category = "Ak Initialization Settings", meta = (ToolTip = "Number of streams to initialize XDSP with for AK Convolution effects with HW Acceleration enabled. Maximum of 256. Less than 16 will leave XDSP uninitialized. Note that each channel of active convolution processing counts as 1 stream. Lower values will reduce memory use. Streams that can't fit will instead use a software fallback.",  ClampMax = 256, UIMax = 256, MinWwiseVersion="2023.1"))
	uint32 MaxXdspStreams = 32;

	UPROPERTY(EditAnywhere, Category = "Ak Initialization Settings", meta = (ToolTip = "Maximum aggregate length of the AK Convolution impulse responses with  HW acceleration enabled that can be running simultaneously, in seconds. Maximum of 128. Lower values will reduce memory use. Streams that can't fit will instead use a software fallback.", ClampMax = 128, UIMax = 128, MinWwiseVersion="2023.1"))
	uint32 MaxXdspAggregateStreamLength = 128;

	UPROPERTY(EditAnywhere, Category = "Ak Initialization Settings", meta = (ToolTip = "Enables XDSP usage on Xbox Lockhart (Series S) consoles. Disabling this will eliminate memory usage of XDSP resources, in favour of increased CPU usage, due to the software fallback that will be used by AK Convolution effects with HW Acceleration enabled."))
	bool EnableXdspOnLockhart = false;

	void FillInitializationStructure(FAkInitializationStructure& InitializationStructure) const;
};


UCLASS(config = Game, defaultconfig)
class AKAUDIO_API UAkXboxSeriesXInitializationSettings : public UAkPlatformInitializationSettingsBase
{
	GENERATED_BODY()

public:
	virtual const TCHAR* GetConfigOverridePlatform() const override
	{
		return TEXT("XSX");
	}

	UAkXboxSeriesXInitializationSettings(const FObjectInitializer& ObjectInitializer);
	void FillInitializationStructure(FAkInitializationStructure& InitializationStructure) const override;

	UPROPERTY(Config, EditAnywhere, Category = "Initialization")
	FAkCommonInitializationSettings CommonSettings;

	UPROPERTY(Config, EditAnywhere, Category = "Initialization")
	FAkCommunicationSettingsWithSystemInitialization CommunicationSettings;

	UPROPERTY(Config, EditAnywhere, Category = "Initialization", AdvancedDisplay)
	FAkXSXAdvancedInitializationSettings AdvancedSettings;

	UFUNCTION()
	void MigrateMultiCoreRendering(bool NewValue)
	{
		AdvancedSettings.EnableMultiCoreRendering = NewValue;
	}
};

UCLASS()
class AKAUDIO_API UAkMPXInitializationSettings : public UAkXboxSeriesXInitializationSettings
{
	GENERATED_BODY()
};

