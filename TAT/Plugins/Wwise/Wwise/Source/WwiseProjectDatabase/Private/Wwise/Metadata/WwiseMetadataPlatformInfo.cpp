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

#include "Wwise/Metadata/WwiseMetadataPlatformInfo.h"
#include "Wwise/Metadata/WwiseMetadataLoader.h"

WwiseMetadataPlatformInfo::WwiseMetadataPlatformInfo(WwiseMetadataLoader& Loader) :
	Platform(Loader.GetLoaderObject<WwiseMetadataPlatform>(this, "Platform"_wwise_db)),
	RootPaths(Loader.GetLoaderObject<WwiseMetadataRootPaths>(this, "RootPaths"_wwise_db)),
	DefaultAlign(Loader.GetWwiseShortId(this, "DefaultAlign"_wwise_db)),
	Settings(Loader.GetLoaderObject<WwiseMetadataSettings>(this, "Settings"_wwise_db)),
	FileHash(Loader.GetGuid(this, "FileHash"_wwise_db))
{
	Loader.LogParsed("PlatformInfo"_wwise_db);
}
