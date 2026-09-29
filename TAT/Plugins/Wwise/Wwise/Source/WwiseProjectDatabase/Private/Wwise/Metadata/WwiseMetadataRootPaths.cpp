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

#include "Wwise/Metadata/WwiseMetadataRootPaths.h"
#include "Wwise/Metadata/WwiseMetadataLoader.h"

WwiseMetadataRootPaths::WwiseMetadataRootPaths()
{
	WWISE_DB_LOG(Error, "Using default WwiseMetadataRootPaths");
}

WwiseMetadataRootPaths::WwiseMetadataRootPaths(WwiseMetadataLoader& Loader) :
	ProjectRoot(Loader.GetString(this, "ProjectRoot"_wwise_db)),
	SourceFilesRoot(Loader.GetString(this, "SourceFilesRoot"_wwise_db)),
	SoundBanksRoot(Loader.GetString(this, "SoundBanksRoot"_wwise_db)),
	ExternalSourcesInputFile(Loader.GetString(this, "ExternalSourcesInputFile"_wwise_db)),
	ExternalSourcesOutputRoot(Loader.GetString(this, "ExternalSourcesOutputRoot"_wwise_db))
{
	Loader.LogParsed("RootPaths"_wwise_db);
}
