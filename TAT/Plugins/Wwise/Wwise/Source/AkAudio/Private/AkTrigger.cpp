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

#include "AkTrigger.h"
#include "Wwise/Stats/AkAudio.h"

#if WITH_EDITORONLY_DATA
#include "Wwise/WwiseProjectDatabase.h"
#include "Wwise/WwiseResourceCooker.h"
#include "AkAudioDevice.h"
#endif

#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
#include "UObject/ObjectSaveContext.h"
#include "Serialization/CompactBinaryWriter.h"
#endif

void UAkTrigger::Serialize(FArchive& Ar)
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
		FWwiseTriggerCookedData CookedDataToArchive;
		if (auto* ResourceCooker = IWwiseResourceCooker::GetForArchive(Ar))
		{
			ResourceCooker->PrepareCookedData(CookedDataToArchive, GetValidatedInfo(TriggerInfo));
		}
		CookedDataToArchive.Serialize(Ar);
	}
#else
	TriggerCookedData.Serialize(Ar);
#endif
#endif
}

#if WITH_EDITOR
void UAkTrigger::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	GetTriggerCookedData();
}
#endif 

#if WITH_EDITORONLY_DATA
void UAkTrigger::FillInfo()
{
	auto* ResourceCooker = IWwiseResourceCooker::GetDefault();
	if (UNLIKELY(!ResourceCooker))
	{
		UE_LOG(LogAkAudio, Error, TEXT("UAkTrigger::FillInfo: ResourceCooker not initialized"));
		return;
	}

	auto ProjectDatabase = ResourceCooker->GetProjectDatabase();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogAkAudio, Error, TEXT("UAkTrigger::FillInfo: ProjectDatabase not initialized"));
		return;
	}

	FWwiseObjectInfo* AudioTypeInfo = &TriggerInfo;
	WwiseRefTrigger TriggerRef = WwiseDataStructureScopeLock(*ProjectDatabase).GetTrigger(
		GetValidatedInfo(TriggerInfo));

	if (TriggerRef.TriggerName()->IsEmpty() || !TriggerRef.TriggerGuid().IsValid() || TriggerRef.TriggerId() == AK_INVALID_UNIQUE_ID)
	{
		UE_LOG(LogAkAudio, Warning, TEXT("UAkTrigger::FillInfo: Valid object not found in Project Database"));
		return;
	}

	int A, B, C, D;
	TriggerRef.TriggerGuid().GetGuidValues(A, B, C, D);
	AudioTypeInfo->WwiseName = FName(**TriggerRef.TriggerName());
	AudioTypeInfo->WwiseGuid = FGuid(A, B, C, D);
	AudioTypeInfo->WwiseShortId = TriggerRef.TriggerId();
}

bool UAkTrigger::ObjectIsInSoundBanks()
{
	auto* ResourceCooker = IWwiseResourceCooker::GetDefault();
	if (UNLIKELY(!ResourceCooker))
	{
		UE_LOG(LogAkAudio, Error, TEXT("UAkTrigger::GetWwiseRef: ResourceCooker not initialized"));
		return false;
	}

	auto ProjectDatabase = ResourceCooker->GetProjectDatabase();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogAkAudio, Error, TEXT("UAkTrigger::GetWwiseRef: ProjectDatabase not initialized"));
		return false;
	}

	FWwiseObjectInfo* AudioTypeInfo = &TriggerInfo;
	WwiseRefTrigger TriggerRef = WwiseDataStructureScopeLock(*ProjectDatabase).GetTrigger(
		GetValidatedInfo(TriggerInfo));

	return TriggerRef.IsValid();
}

void UAkTrigger::GetTriggerCookedData()
{
	SCOPED_AKAUDIO_EVENT_2(TEXT("GetTriggerCookedData"));
	if (!IWwiseProjectDatabaseModule::ShouldInitializeProjectDatabase())
	{
		return;
	}
	auto* ProjectDatabase = FWwiseProjectDatabase::Get();
	if (!ProjectDatabase || !ProjectDatabase->IsProjectDatabaseParsed())
	{
		UE_LOG(LogAkAudio, VeryVerbose, TEXT("UAkTrigger::GetTriggerCookedData: Not loading '%s' because project database is not parsed."), *GetName())
		return;
	}
	auto* ResourceCooker = IWwiseResourceCooker::GetDefault();
	if (UNLIKELY(!ResourceCooker))
	{
		return;
	}
	if(!ResourceCooker->PrepareCookedData(TriggerCookedData, GetValidatedInfo(TriggerInfo)))
	{
		const auto* AudioDevice = FAkAudioDevice::Get();
		if( AudioDevice && AudioDevice->IsWwiseProfilerConnected())
		{
			UE_LOG(LogAkAudio, Verbose, TEXT("Could not fetch CookedData for Trigger %s, but Wwise profiler is connected. Previous errors can be ignored."),
			*GetName());
		}
		else
		{
			return;
		}
	}
}
#endif

#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
UE_COOK_DEPENDENCY_FUNCTION(HashWwiseTriggerDependenciesForCook, UAkAudioType::HashDependenciesForCook);

void UAkTrigger::PreSave(FObjectPreSaveContext SaveContext)
{
	ON_SCOPE_EXIT
	{
		Super::PreSave(SaveContext);
	};

	if (!SaveContext.IsCooking())
	{
		return;
	}

	auto* ResourceCooker = IWwiseResourceCooker::GetForPlatform(SaveContext.GetTargetPlatform());
	if (UNLIKELY(!ResourceCooker))
	{
		return;
	}

	FWwiseTriggerCookedData CookedDataToArchive;
	ResourceCooker->PrepareCookedData(CookedDataToArchive, GetValidatedInfo(TriggerInfo));
	FillMetadata(ResourceCooker->GetProjectDatabase());

	FCbWriter Writer;
	Writer.BeginObject();
	CookedDataToArchive.PreSave(SaveContext, Writer);
	Writer.EndObject();
	
	SaveContext.AddCookBuildDependency(
		UE::Cook::FCookDependency::Function(
			UE_COOK_DEPENDENCY_FUNCTION_CALL(HashWwiseTriggerDependenciesForCook), Writer.Save()));
}
#endif
