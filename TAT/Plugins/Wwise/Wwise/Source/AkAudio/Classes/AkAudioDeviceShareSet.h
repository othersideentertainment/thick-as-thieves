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

#include "AkAudioType.h"
#include "Wwise/CookedData/WwiseAudioDeviceShareSetCookedData.h"
#include "Wwise/Loaded/WwiseLoadedShareSet.h"
#if WITH_EDITORONLY_DATA
#include "Wwise/Info/WwiseObjectInfo.h"
#endif
#include "AkAudioDeviceShareSet.generated.h"

UCLASS(BlueprintType)
class AKAUDIO_API UAkAudioDeviceShareSet : public UAkAudioType
{
	GENERATED_BODY()
public:

	UPROPERTY(Transient, VisibleAnywhere, Category = "AkAudioDeviceShareSet")
		FWwiseAudioDeviceShareSetCookedData AudioDeviceShareSetCookedData;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "AkAudioDeviceShareSet")
		FWwiseObjectInfo AudioDeviceShareSetInfo;
#endif

public:
	void GetAudioDeviceShareSetCookedData();
	void Serialize(FArchive& Ar) override;
	virtual AkUInt32 GetShortID() const override { return AudioDeviceShareSetCookedData.ShortId; }
	virtual void LoadData()   override { GetAudioDeviceShareSetCookedData(); }


#if WITH_EDITORONLY_DATA
	void PostLoad() override;
	virtual FWwiseObjectInfo* GetInfoMutable() override { return &AudioDeviceShareSetInfo; };
	virtual FWwiseObjectInfo GetInfo() const override { return AudioDeviceShareSetInfo; }
	virtual void FillInfo() override;
#endif
};
