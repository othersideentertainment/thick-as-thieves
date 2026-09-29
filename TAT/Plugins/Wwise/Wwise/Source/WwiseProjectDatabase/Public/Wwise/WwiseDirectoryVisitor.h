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

#include "Wwise/WwiseGeneratedFiles.h"

#include "Async/Future.h"
#include "CoreTypes.h"
#include "Containers/UnrealString.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Wwise/AdapterTypes/IWwiseDirectoryVisitor.h"

class WWISEPROJECTDATABASE_API WwiseDirectoryVisitor
{
	friend IWwiseDirectoryVisitor<WwiseDirectoryVisitor>;
public:
	WwiseDirectoryVisitor(const WwiseDBString* InPlatformName = nullptr) :
		PlatformName(InPlatformName)
	{}

	WwiseGeneratedFiles& Get();

protected:
	bool Visit(const WwiseDBString& FilenameOrDirectory, bool bIsDirectory);

private:
	WwiseGeneratedFiles GeneratedDirectory;
	
	class FPlatformRootDirectoryVisitor;

	WwiseDBArray<WwiseDBFuture<FPlatformRootDirectoryVisitor*>> Futures;

	const WwiseDBString* PlatformName;

	class FSoundBankVisitor;
	class FMediaVisitor;
	WwiseDirectoryVisitor& operator=(const WwiseDirectoryVisitor& Rhs) = delete;
	WwiseDirectoryVisitor(const WwiseDirectoryVisitor& Rhs) = delete;
};
