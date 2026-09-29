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
#include <filesystem>

#include "Wwise/WwiseGeneratedFiles.h"

template<typename Type>
class IWwiseDirectoryVisitor
{
public:
	Type* DirectoryVisitor;

	IWwiseDirectoryVisitor(Type* DirectoryVisitor) : DirectoryVisitor(DirectoryVisitor){}
	
	bool IterateDirectory(const WwiseDBString& InPath, bool bIsRecursive = false)
	{
		if(!std::filesystem::exists(*InPath))
		{
			return false;
		}
		bool bResult = true;
		if(bIsRecursive)
		{
			for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(*InPath))
			{
				if(!DirectoryVisitor->Visit(WwiseDBString((const char*)dirEntry.path().u8string().c_str()), dirEntry.is_directory()))
				{
					bResult = false;
				}
			}
		}
		else
		{
			for (const auto& dirEntry : std::filesystem::directory_iterator(*InPath))
			{
				if(!DirectoryVisitor->Visit(WwiseDBString((const char*)dirEntry.path().u8string().c_str()), dirEntry.is_directory()))
				{
					bResult = false;
				}
			}
		}

		return bResult;
	}

	auto Get()
	{
		return DirectoryVisitor->Get();
	}

	static std::filesystem::file_time_type GetTimeStamp(const WwiseDBString& InPath)
	{
		if(!std::filesystem::exists(*InPath))
		{
			return {};
		}
		return std::filesystem::last_write_time(*InPath);
	}
};
