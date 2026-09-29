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

#include "AkAudioDeviceShareSet.h"

#if WITH_EDITORONLY_DATA
#include "Wwise/WwiseProjectDatabase.h"
#include "Wwise/WwiseResourceCooker.h"
#include "AkAudioDevice.h"
#endif

void UAkAudioDeviceShareSet::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}
#if !UE_SERVER
#if WITH_EDITORONLY_DATA
	if (Ar.IsCooking() && Ar.IsSaving() && !Ar.CookingTarget()->IsServerOnly())
	{
		FWwiseAudioDeviceShareSetCookedData CookedDataToArchive;
		if (auto* ResourceCooker = IWwiseResourceCooker::GetForArchive(Ar))
		{
			ResourceCooker->PrepareCookedData(CookedDataToArchive, GetValidatedInfo(AudioDeviceShareSetInfo));
		}
		CookedDataToArchive.Serialize(Ar);
	}
#else
	AudioDeviceShareSetCookedData.Serialize(Ar);
#endif
#endif
}

#if WITH_EDITORONLY_DATA
void UAkAudioDeviceShareSet::PostLoad()
{
	Super::PostLoad();
	GetAudioDeviceShareSetCookedData();
}

void UAkAudioDeviceShareSet::FillInfo()
{
	auto* ResourceCooker = IWwiseResourceCooker::GetDefault();
	if (UNLIKELY(!ResourceCooker))
	{
		UE_LOG(LogAkAudio, Error, TEXT("UAkAudioDeviceShareSet::FillInfo: ResourceCooker not initialized"));
		return;
	}

	auto ProjectDatabase = ResourceCooker->GetProjectDatabase();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogAkAudio, Error, TEXT("UAkAudioDeviceShareSet::FillInfo: ProjectDatabase not initialized"));
		return;
	}

	FWwiseObjectInfo* AudioTypeInfo = &AudioDeviceShareSetInfo;
	const WwiseRefAudioDevice AudioTypeRef = WwiseDataStructureScopeLock(*ProjectDatabase).GetAudioDevice(
		GetValidatedInfo(AudioDeviceShareSetInfo));

	if (AudioTypeRef.AudioDeviceName()->IsEmpty() || !AudioTypeRef.AudioDeviceGuid().IsValid() || AudioTypeRef.AudioDeviceId() == AK_INVALID_UNIQUE_ID)
	{
		UE_LOG(LogAkAudio, Warning, TEXT("UAkAudioDeviceShareSet::FillInfo: Valid object not found in Project Database"));
		return;
	}

	AudioTypeInfo->WwiseName = FName(**AudioTypeRef.AudioDeviceName());
	int A, B, C, D;
	AudioTypeRef.AudioDeviceGuid().GetGuidValues(A, B, C, D);
	AudioTypeInfo->WwiseGuid = FGuid(A, B, C, D);
	AudioTypeInfo->WwiseShortId = AudioTypeRef.AudioDeviceId();
}
#endif

void UAkAudioDeviceShareSet::GetAudioDeviceShareSetCookedData()
{
#if WITH_EDITORONLY_DATA
	if (!IWwiseProjectDatabaseModule::ShouldInitializeProjectDatabase())
	{
		return;
	}
	auto* ProjectDatabase = FWwiseProjectDatabase::Get();
	if (!ProjectDatabase || !ProjectDatabase->IsProjectDatabaseParsed())
	{
		UE_LOG(LogAkAudio, VeryVerbose, TEXT("UAkAudioDeviceShareSet::GetAudioDeviceShareSetCookedData: Not loading '%s' because project database is not parsed."), *GetName())
			return;
	}
	auto* ResourceCooker = IWwiseResourceCooker::GetDefault();
	if (UNLIKELY(!ResourceCooker))
	{
		return;
	}

	ResourceCooker->PrepareCookedData(AudioDeviceShareSetCookedData, GetValidatedInfo(AudioDeviceShareSetInfo));
#endif
}