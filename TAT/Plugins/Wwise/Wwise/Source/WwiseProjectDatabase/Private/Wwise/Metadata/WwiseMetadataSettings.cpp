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

#include "Wwise/Metadata/WwiseMetadataSettings.h"
#include "Wwise/Metadata/WwiseMetadataLoader.h"

WwiseMetadataSettings::WwiseMetadataSettings() :
	bAutoSoundBankDefinition(false),
	bCopyLooseStreamedMediaFiles(false),
	bSubFoldersForGeneratedFiles(false),
	bRemoveUnusedGeneratedFiles(false),
	bSourceControlGeneratedFiles(false),
	bGenerateHeaderFile(false),
	bGenerateContentTxtFile(false),
	bGenerateMetadataXML(false),
	bGenerateMetadataJSON(false),
	bGenerateAllBanksMetadata(false),
	bGeneratePerBankMetadata(false),
	bUseSoundBankNames(false),
	bAllowExceedingMaxSize(false),
	bMaxAttenuationInfo(false),
	bEstimatedDurationInfo(false),
	bPrintObjectGuid(false),
	bPrintObjectPath(false),
	bPrintObjectColor(false)
{
	WWISE_DB_LOG(Error, "Using default Settings");
}

WwiseMetadataSettings::WwiseMetadataSettings(WwiseMetadataLoader& Loader) :
	bAutoSoundBankDefinition(Loader.GetBool(this, "AutoSoundBankDefinition"_wwise_db)),
	bCopyLooseStreamedMediaFiles(Loader.GetBool(this, "CopyLooseStreamedMediaFiles"_wwise_db)),
	bSubFoldersForGeneratedFiles(Loader.GetBool(this, "SubFoldersForGeneratedFiles"_wwise_db)),
	bRemoveUnusedGeneratedFiles(Loader.GetBool(this, "RemoveUnusedGeneratedFiles"_wwise_db)),
	bSourceControlGeneratedFiles(Loader.GetBool(this, "SourceControlGeneratedFiles"_wwise_db)),
	bGenerateHeaderFile(Loader.GetBool(this, "GenerateHeaderFile"_wwise_db)),
	bGenerateContentTxtFile(Loader.GetBool(this, "GenerateContentTxtFile"_wwise_db)),
	bGenerateMetadataXML(Loader.GetBool(this, "GenerateMetadataXML"_wwise_db)),
	bGenerateMetadataJSON(Loader.GetBool(this, "GenerateMetadataJSON"_wwise_db)),
	bGenerateAllBanksMetadata(Loader.GetBool(this, "GenerateAllBanksMetadata"_wwise_db)),
	bGeneratePerBankMetadata(Loader.GetBool(this, "GeneratePerBankMetadata"_wwise_db)),
	bUseSoundBankNames(Loader.GetBool(this, "UseSoundBankNames"_wwise_db)),
	bAllowExceedingMaxSize(Loader.GetBool(this, "AllowExceedingMaxSize"_wwise_db)),
	bMaxAttenuationInfo(Loader.GetBool(this, "MaxAttenuationInfo"_wwise_db)),
	bEstimatedDurationInfo(Loader.GetBool(this, "EstimatedDurationInfo"_wwise_db)),
	bPrintObjectGuid(Loader.GetBool(this, "PrintObjectGuid"_wwise_db)),
	bPrintObjectPath(Loader.GetBool(this, "PrintObjectPath"_wwise_db)),
	bPrintObjectColor(Loader.GetBool(this, "PrintObjectColor"_wwise_db, WwiseRequiredMetadata::Optional))
{
	Loader.LogParsed("Settings"_wwise_db);
}
