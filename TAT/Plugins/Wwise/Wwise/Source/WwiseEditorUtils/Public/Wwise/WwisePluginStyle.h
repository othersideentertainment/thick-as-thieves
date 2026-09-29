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

#include "CoreMinimal.h"
#include "Styling/ISlateStyle.h"
#include "Styling/SlateStyle.h"

class WWISEEDITORUTILS_API FWwisePluginStyle : public FSlateStyleSet
{
protected:
	FWwisePluginStyle();

public:
	static TSharedPtr< FWwisePluginStyle > Get();
	static void InitializeWwisePluginStyle();
	static void ShutdownWwisePluginStyle();

	static const FName AcousticTextureIconName;
	static const FName ActorMixerIconName;
	static const FName AssetLibraryIconName;
	static const FName AssetLibraryFilterIconName;
	static const FName AudioDeviceIconName;
	static const FName AuxBusIconName;
	static const FName BlendContainerIconName;
	static const FName BusIconName;
	static const FName EffectShareSetIconName;
	static const FName EventIconName;
	static const FName ExternalSourceIconName;
	static const FName FolderIconName;
	static const FName GameParameterIconName;
	static const FName InitBankIconName;
	static const FName MediaIconName;
	static const FName MotionBusIconName;
	static const FName PhysicalFolderIconName;
	static const FName ProjectIconName;
	static const FName RandomSequenceContainerIconName;
	static const FName StateGroupIconName;
	static const FName StateIconName;
	static const FName SoundBankIconName;
	static const FName SoundIconName;
	static const FName SwitchContainerIconName;
	static const FName SwitchGroupIconName;
	static const FName SwitchIconName;
	static const FName TriggerIconName;
	static const FName WorkUnitIconName;
	static const FName WwiseBrowserTabIconName;
	static const FName WwiseIconName;

protected:
	static TSharedPtr< FWwisePluginStyle > StyleInstance;

	void SetV(const FName& BrushName, const FString& TextureName, const FVector2D& Size);
	void SetI(const FName& BrushName, const FString& TextureName, const FVector2D& Size);
	virtual void Initialize();
	virtual void Shutdown();
};
