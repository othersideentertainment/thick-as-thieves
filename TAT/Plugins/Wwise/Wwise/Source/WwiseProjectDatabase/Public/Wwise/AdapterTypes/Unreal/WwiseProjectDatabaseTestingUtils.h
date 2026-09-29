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

#include "Wwise/WwiseUnitTests.h"

#ifndef SCENARIO
#define WWISE_TEST_CASE_PROJECT_DATABASE_NAME(Label, File, Counter) Label##File##Counter 
#define WWISE_TEST_CASE_PROJECT_DATABASE_NAME_2(Label, File, Counter) WWISE_TEST_CASE_PROJECT_DATABASE_NAME(Label, File, Counter)
#define SCENARIO(PrettyName) WWISE_TEST_CASE_PROJECT_DATABASE(WWISE_TEST_CASE_PROJECT_DATABASE_NAME_2(WwiseProjectDatabaseTests_, __COUNTER__, __LINE__), PrettyName)
#define WWISE_TEST_CASE_PROJECT_DATABASE(ClassName, PrettyName) WWISE_TEST_CASE(ClassName, PrettyName, "[ApplicationContextMask][SmokeFilter]")

#define PROJECT_DATABASE_TEST_CASE_2(ClassName, PrettyName) WWISE_TEST_CASE(ClassName, PrettyName, "[NegativeFilter]")
#define PROJECT_DATABASE_TEST_CASE(PrettyName, tags) PROJECT_DATABASE_TEST_CASE_2(WWISE_TEST_CASE_PROJECT_DATABASE_NAME_2(WwiseProjectDatabaseTests_, __COUNTER__, __LINE__), PrettyName)
#endif

namespace WwiseProjectDatabaseLoggingUtils
{
	inline void ResetErrors(){}

	inline void ParseErrors(){}

	inline void AddExpectedError(const char* Message, int occurences = 1)
	{
		FAutomationTestFramework::Get().GetCurrentTest()->AddExpectedError(Message, EAutomationExpectedErrorFlags::Contains, occurences);
	}
}