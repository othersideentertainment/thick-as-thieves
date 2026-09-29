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

#include "Wwise/Metadata/WwiseMetadataProjectInfo.h"

#include "Wwise/Metadata/WwiseMetadataLanguage.h"
#include "Wwise/Metadata/WwiseMetadataLoader.h"
#include "Wwise/Metadata/WwiseMetadataPlatform.h"

WwiseMetadataProjectInfo::WwiseMetadataProjectInfo(WwiseMetadataLoader& Loader) :
	Project(Loader.GetLoaderObject<WwiseMetadataProject>(this, "Project"_wwise_db)),
	CacheRoot(Loader.GetString(this, "CacheRoot"_wwise_db)),
	Platforms(Loader.GetArray<WwiseMetadataPlatformReference>(this, "Platforms"_wwise_db)),
	Languages(Loader.GetArray<WwiseMetadataLanguage>(this, "Languages"_wwise_db)),
	FileHash(Loader.GetGuid(this, "FileHash"_wwise_db))
{
	Loader.LogParsed("ProjectInfo"_wwise_db);
}
