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

#include "Wwise/WwiseUnitTests.h"
#include "Wwise/AdapterTypes/WwiseWrapperTypes.h"
#include "Wwise/WwiseStringConverter.h"

#if WWISE_UNIT_TESTS
WWISE_TEST_CASE(WwiseProjectDatabaseUnreal, "Wwise::WwiseProjectDatabase::Unreal", "[ApplicationContextMask][SmokeFilter]")
{
#ifdef WWISE_DB_STANDARD_TYPES
	SECTION("StandardStringConversion")
	{
		WwiseStandardString String("Test");
		WwiseStandardString Path("Path/To/../To/Test.exe");
		WwiseStandardString EmojiTest("\xF0\x9F\x8C\xAD");

		FString Cafe = TEXT("Café");

		FString UnrealFString = FWwiseStringConverter::ToFString(String);
		FString PathFString = FWwiseStringConverter::ToFString(Path);
		FString EmojiFString = FWwiseStringConverter::ToFString(EmojiTest);

		CHECK(UnrealFString == FString(TEXT("Test")))
		CHECK(PathFString == FString(TEXT("Path/To/../To/Test.exe")))
		// Unreal strings are UTF-16 by default
		CHECK(EmojiFString == FString(TEXT("\xD83C\xDF2D")))


		WwiseStandardString CafeWwiseString = FWwiseStringConverter::ToWwiseDBString(Cafe);
		CHECK(CafeWwiseString == WwiseStandardString("Café"))

	}

#endif // WWISE_DB_STANDARD_TYPES

#ifdef WWISE_DB_UNREAL_TYPES	
	SECTION("WwiseUnrealStringConversion")
	{
		WwiseUnrealString String("Test");
		WwiseUnrealString Path("Path/To/../To/Test.exe");
		WwiseUnrealString EmojiTest(TEXT("\xD83C\xDF2D"));

		FString Cafe = TEXT("Café");

		FString UnrealFString = FWwiseStringConverter::ToFString(String);
		FString PathFString = FWwiseStringConverter::ToFString(Path);
		FString EmojiFString = FWwiseStringConverter::ToFString(EmojiTest);

		CHECK(UnrealFString == FString(TEXT("Test")))
		CHECK(PathFString == FString(TEXT("Path/To/../To/Test.exe")))
		// Unreal strings are UTF-16 by default
		CHECK(EmojiFString == FString(TEXT("\xD83C\xDF2D")))
		
		WwiseUnrealString CafeWwiseString = FWwiseStringConverter::ToWwiseDBString(Cafe);
		CHECK(CafeWwiseString == WwiseUnrealString(TEXT("Café")))
	}

#endif // WWISE_DB_UNREAL_TYPES

}
#endif // WWISE_UNIT_TESTS
