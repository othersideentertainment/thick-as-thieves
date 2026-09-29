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
#include "Wwise/AdapterTypes/WwiseProjectDatabaseLoggingSeverity.h"
#include "Wwise/Stats/ProjectDatabase.h"


inline void UnrealLogging(bool bCondition, WwiseProjectDatabaseLoggingSeverity Severity, const char* Format, ...)
{
	
	if(!bCondition)
	{
		return;
	}

#ifdef WWISE_DB_STANDARD_TYPES
	char FormattedString[1024];
	va_list Args;
	va_start(Args, Format);
	FCStringAnsi::GetVarArgs(FormattedString, UE_ARRAY_COUNT(FormattedString), Format, Args);
	va_end(Args);
	
	const auto UnrealString = StringCast<TCHAR>((UTF8CHAR*)FormattedString);

	switch(Severity)
	{
	case Fatal:
		UE_LOG(LogWwiseProjectDatabase, Fatal, TEXT("%s"), UnrealString.Get());
		break;
	case Error:
		UE_LOG(LogWwiseProjectDatabase, Error, TEXT("%s"), UnrealString.Get());
		break;
	case Warning:
		UE_LOG(LogWwiseProjectDatabase, Warning, TEXT("%s"), UnrealString.Get());
		break;
	case Log:
		UE_LOG(LogWwiseProjectDatabase, Log, TEXT("%s"), UnrealString.Get());
		break;
	case Display:
		UE_LOG(LogWwiseProjectDatabase, Display, TEXT("%s"), UnrealString.Get());
		break;
	case Verbose:
		UE_LOG(LogWwiseProjectDatabase, Verbose, TEXT("%s"), UnrealString.Get());
		break;
	case VeryVerbose:
		UE_LOG(LogWwiseProjectDatabase, VeryVerbose, TEXT("%s"), UnrealString.Get());
		break;
	default:
		break;
	}

#elif defined(WWISE_DB_UNREAL_TYPES) 
	TCHAR FormattedString[1024];
	va_list Args;
	va_start(Args, Format);
	auto Converter = StringCast<TCHAR>(Format);
	const TCHAR* ConvertedFmt = Converter.Get();
	FCString::GetVarArgs(FormattedString, UE_ARRAY_COUNT(FormattedString), ConvertedFmt, Args);
	va_end(Args);
	
	switch(Severity)
	{
	case Fatal:
		UE_LOG(LogWwiseProjectDatabase, Fatal, TEXT("%s"), FormattedString);
		break;
	case Error:
		UE_LOG(LogWwiseProjectDatabase, Error, TEXT("%s"), FormattedString);
		break;
	case Warning:
		UE_LOG(LogWwiseProjectDatabase, Warning, TEXT("%s"), FormattedString);
		break;
	case Log:
		UE_LOG(LogWwiseProjectDatabase, Log, TEXT("%s"), FormattedString);
		break;
	case Display:
		UE_LOG(LogWwiseProjectDatabase, Display, TEXT("%s"), FormattedString);
		break;
	case Verbose:
		UE_LOG(LogWwiseProjectDatabase, Verbose, TEXT("%s"), FormattedString);
		break;
	case VeryVerbose:
		UE_LOG(LogWwiseProjectDatabase, VeryVerbose, TEXT("%s"), FormattedString);
		break;
	default:
		break;
	}

#endif
}
