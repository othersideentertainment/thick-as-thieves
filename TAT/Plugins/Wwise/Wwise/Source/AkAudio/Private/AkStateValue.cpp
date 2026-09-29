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

#include "AkStateValue.h"

#include "Wwise/WwiseResourceLoader.h"
#include "Wwise/Stats/AkAudio.h"

#if WITH_EDITORONLY_DATA
#include "Wwise/WwiseProjectDatabase.h"
#include "Wwise/WwiseResourceCooker.h"
#include "AkAudioDevice.h"
#endif

void UAkStateValue::LoadGroupValue()
{
	SCOPED_AKAUDIO_EVENT_2(TEXT("UAkStateValue::LoadGroupValue"));
	FWwiseResourceLoaderPtr ResourceLoader = FWwiseResourceLoader::Get();
	if (UNLIKELY(!ResourceLoader))
	{
		return;
	}
	
	UnloadGroupValue(false);

#if WITH_EDITORONLY_DATA
	if (!IWwiseProjectDatabaseModule::ShouldInitializeProjectDatabase())
	{
		return;
	}
	auto* ProjectDatabase = FWwiseProjectDatabase::Get();
	if (!ProjectDatabase || !ProjectDatabase->IsProjectDatabaseParsed())
	{
		UE_LOG(LogAkAudio, VeryVerbose, TEXT("UAkStateValue::LoadGroupValue: Not loading '%s' because project database is not parsed."), *GetName())
		return;
	}
	auto* ResourceCooker = IWwiseResourceCooker::GetDefault();
	if (UNLIKELY(!ResourceCooker))
	{
		return;
	}
	if (!ResourceCooker->PrepareCookedData(GroupValueCookedData, GetValidatedInfo(GroupValueInfo), EWwiseGroupType::State))
	{
		const auto* AudioDevice = FAkAudioDevice::Get();
		if( AudioDevice && AudioDevice->IsWwiseProfilerConnected())
		{
			UE_LOG(LogAkAudio, Verbose, TEXT("Could not fetch CookedData for State Value %s, but Wwise profiler is connected. Previous errors can be ignored."),
			*GetName());
		}
		else
		{
			return;
		}
	}
#endif
	
	const auto NewlyLoadedGroupValue = ResourceLoader->LoadGroupValue(GroupValueCookedData);
	auto PreviouslyLoadedGroupValue = LoadedGroupValue.exchange(NewlyLoadedGroupValue);
	if (UNLIKELY(PreviouslyLoadedGroupValue))
	{
		ResourceLoader->UnloadGroupValue(MoveTemp(PreviouslyLoadedGroupValue));
	}
}

void UAkStateValue::Serialize(FArchive& Ar)
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
		FWwiseGroupValueCookedData CookedDataToArchive;
		if (auto* ResourceCooker = IWwiseResourceCooker::GetForArchive(Ar))
		{
			ResourceCooker->PrepareCookedData(CookedDataToArchive, GetValidatedInfo(GroupValueInfo), EWwiseGroupType::State);
		}
		CookedDataToArchive.Serialize(Ar);
	}
#else
	GroupValueCookedData.Serialize(Ar);
#endif
#endif
}

#if WITH_EDITORONLY_DATA
void UAkStateValue::FillInfo()
{
	auto* ResourceCooker = IWwiseResourceCooker::GetDefault();
	if (UNLIKELY(!ResourceCooker))
	{
		UE_LOG(LogAkAudio, Error, TEXT("UAkStateValue::FillInfo: ResourceCooker not initialized"));
		return;
	}

	auto ProjectDatabase = ResourceCooker->GetProjectDatabase();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogAkAudio, Error, TEXT("UAkStateValue::FillInfo: ProjectDatabase not initialized"));
		return;
	}

	FWwiseGroupValueInfo* AudioTypeInfo = static_cast<FWwiseGroupValueInfo*>(GetInfoMutable());
	WwiseRefState RefState = WwiseDataStructureScopeLock(*ProjectDatabase).GetState(
		GetValidatedInfo(*AudioTypeInfo));

	if (RefState.StateName().IsEmpty() || !RefState.StateGuid().IsValid() || RefState.StateId() == AK_INVALID_UNIQUE_ID)
	{
		UE_LOG(LogAkAudio, Warning, TEXT("UAkStateValue::FillInfo: Valid object not found in Project Database"));
		return;
	}

	int A, B, C, D;
	RefState.StateGuid().GetGuidValues(A, B, C, D);
	
	AudioTypeInfo->WwiseName = FName(*RefState.StateName());
	AudioTypeInfo->WwiseGuid = FGuid(A, B, C, D);
	AudioTypeInfo->WwiseShortId = RefState.StateId();
	AudioTypeInfo->GroupShortId = RefState.StateGroupId();
}

void UAkStateValue::FillInfo(const WwiseAnyRef& CurrentWwiseRef)
{
	FWwiseGroupValueInfo* AudioTypeInfo = static_cast<FWwiseGroupValueInfo*>(GetInfoMutable());

	int A, B, C, D;
	CurrentWwiseRef.GetGuid().GetGuidValues(A, B, C, D);
	AudioTypeInfo->WwiseName = FName(*CurrentWwiseRef.GetName());
	AudioTypeInfo->WwiseGuid = FGuid(A, B, C, D);;
	AudioTypeInfo->WwiseShortId = CurrentWwiseRef.GetId();
	AudioTypeInfo->GroupShortId = CurrentWwiseRef.GetGroupId();
}

FName UAkStateValue::GetWwiseGroupName()
{
	auto* ResourceCooker = IWwiseResourceCooker::GetDefault();
	if (UNLIKELY(!ResourceCooker))
	{
		UE_LOG(LogAkAudio, Error, TEXT("UAkStateValue::FillInfo: ResourceCooker not initialized"));
		return {};
	}

	auto ProjectDatabase = ResourceCooker->GetProjectDatabase();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogAkAudio, Error, TEXT("UAkStateValue::FillInfo: ProjectDatabase not initialized"));
		return {};
	}

	FWwiseGroupValueInfo* AudioTypeInfo = static_cast<FWwiseGroupValueInfo*>(GetInfoMutable());
	WwiseRefState RefState = WwiseDataStructureScopeLock(*ProjectDatabase).GetState(
		GetValidatedInfo(*AudioTypeInfo));

	return FName(**RefState.StateGroupName());
}

bool UAkStateValue::ObjectIsInSoundBanks()
{
	auto* ResourceCooker = IWwiseResourceCooker::GetDefault();
	if (UNLIKELY(!ResourceCooker))
	{
		UE_LOG(LogAkAudio, Error, TEXT("UAkStateValue::GetWwiseRef: ResourceCooker not initialized"));
		return false;
	}

	auto ProjectDatabase = ResourceCooker->GetProjectDatabase();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogAkAudio, Error, TEXT("UAkStateValue::GetWwiseRef: ProjectDatabase not initialized"));
		return false;
	}

	FWwiseGroupValueInfo* AudioTypeInfo = static_cast<FWwiseGroupValueInfo*>(GetInfoMutable());
	WwiseRefState RefState = WwiseDataStructureScopeLock(*ProjectDatabase).GetState(
		GetValidatedInfo(*AudioTypeInfo));

	return RefState.IsValid();
}
#endif
