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

#include "CoreMinimal.h"
#include "Containers/StringConv.h"
#include "Wwise/AdapterTypes/WwiseWrapperTypes.h"

namespace FWwiseStringConverter
{
#ifdef WWISE_DB_STANDARD_TYPES
	// Returns a new FString with the contents converted from InString
	inline FString ToFString(const WwiseStandardString& InString)
	{
		return FString((const UTF8CHAR*) *InString);
	}

	// Returns a new UTF-8 encoded WwiseStandard string with the contents converted from InString
	inline WwiseStandardString ToWwiseDBString(const FString& InString)
	{
		const TCHAR* StringData = *InString;
		auto Converter = StringCast<UTF8CHAR>(StringData);
		return (char*)Converter.Get();
	}
	
#elif defined(WWISE_DB_UNREAL_TYPES)
	inline FString ToFString(const WwiseUnrealString& InString)
	{
		return *InString;
	}
	
	inline WwiseUnrealString ToWwiseDBString(const FString& InString)
	{
		return InString;
	}
#endif
	
};
