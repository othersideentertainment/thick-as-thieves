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

#include "Containers/UnrealString.h"
#include "Misc/Paths.h"
#include "Wwise/AdapterTypes/WwiseString.h"

#ifdef WWISE_DB_TEXT
#error Cannot Use both WwiseUnrealString and WwiseStandardString
#endif

// An implementation of IWwiseString which holds an Unreal TCHAR* string for use within the Wwise Project Database
class WwiseUnrealString : public IWwiseString<FString>
{
public:
	WwiseUnrealString(){}
	
	WwiseUnrealString(const FString& InString)
	{
		String = InString;
	}

	WwiseUnrealString(const TCHAR* InChars)
	{
		String = FString(InChars);
	}

	bool IsEmpty() const override
	{
		return String.IsEmpty();
	}

	unsigned int Size() const override
	{
		return String.Len();
	}

	unsigned int AllocatedSize() const override
	{
		return String.GetAllocatedSize();
	}

	bool operator<(const WwiseUnrealString& Rhs) const
	{
		return this->String < Rhs.String;
	}

	bool operator==(const WwiseUnrealString& Rhs) const
	{
		return this->String == Rhs.String;
	}
	
	bool operator!=(const WwiseUnrealString& Rhs) const
	{
		return !(*this == Rhs.String);
	}
	
	const TCHAR* operator*() const
	{
		return *String;
	}

	WwiseUnrealString operator/(const WwiseUnrealString& Rhs) const
	{
		return WwiseUnrealString(String / Rhs.String);
	}

	void MakeStandardFilename()
	{
		FPaths::MakeStandardFilename(String);
	}

	WwiseUnrealString GetFileName() const
	{
		return FPaths::GetCleanFilename(String);
	}

	WwiseUnrealString GetExtension() const
	{
		return FPaths::GetExtension(String);
	}

	WwiseUnrealString GetPath() const
	{
		return FPaths::GetPath(String);
	}

	void CollapseRelativeDirectories()
	{
		FPaths::CollapseRelativeDirectories(String);
	}

	bool IsRelative() const
	{
		return FPaths::IsRelative(String);
	}

	bool StartsWith(const WwiseUnrealString& InString) const
	{
		return String.StartsWith(*InString);
	}

	bool Contains(const WwiseUnrealString& InString) const
	{
		return String.Contains(*InString);
	}

	bool Equals(const WwiseUnrealString& Rhs, EStringCompareType Compare) const
	{
		return String.Equals(Rhs.String, Compare == CaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase);
	}
};

inline uint32 GetTypeHash(const WwiseUnrealString& InString)
{
	return GetTypeHash(InString.String);
}

inline WwiseUnrealString operator ""_wwise_db(const char* c, size_t size)
{
	return WwiseUnrealString{c};
}
