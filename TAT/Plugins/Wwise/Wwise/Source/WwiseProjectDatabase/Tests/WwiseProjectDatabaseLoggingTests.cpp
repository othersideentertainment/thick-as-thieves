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

#ifdef TESTING_WWISE_PROJECT_DATABASE
#include <catch2/catch.hpp>
#include "WwiseProjectDatabase.h"
#include "AdapterTypes/WwiseWrapperTypes.h"
#define WWISE_PROJECT_DATABASE_TEST_CASE(name, tags) TEST_CASE(name, tags)
#else
#include "Wwise/WwiseProjectDatabase.h"
#include "Wwise/AdapterTypes/WwiseWrapperTypes.h"
#endif
#include "Wwise/AdapterTypes/Unreal/WwiseProjectDatabaseTestingUtils.h"


PROJECT_DATABASE_TEST_CASE("Wwise::WwiseProjectDatabase::Logging::UnhandledError", "[!shouldfail]")
{
    WwiseProjectDatabaseLoggingUtils::ResetErrors();
    WWISE_DB_LOG(Error, "An Error");
    WwiseProjectDatabaseLoggingUtils::ParseErrors();
}

PROJECT_DATABASE_TEST_CASE("Wwise::WwiseProjectDatabase::Logging::Unhandled Expected Error", "[!shouldfail]")
{
    WwiseProjectDatabaseLoggingUtils::ResetErrors();
    WwiseProjectDatabaseLoggingUtils::AddExpectedError("An Error");
    WwiseProjectDatabaseLoggingUtils::ParseErrors();
}

PROJECT_DATABASE_TEST_CASE("Wwise::WwiseProjectDatabase::Logging::Unhandled Expected Error with Multiple Occurences", "[!shouldfail]")
{
    WwiseProjectDatabaseLoggingUtils::ResetErrors();
    WwiseProjectDatabaseLoggingUtils::AddExpectedError("An Error", 2);
    WWISE_DB_LOG(Error, "An Error");
    WwiseProjectDatabaseLoggingUtils::ParseErrors();
}

SCENARIO("Wwise::WwiseProjectDatabase::Logging")
{
    SECTION("Expected Error")
    {
        WwiseProjectDatabaseLoggingUtils::ResetErrors();
        WwiseProjectDatabaseLoggingUtils::AddExpectedError("An Error");
        WWISE_DB_LOG(Error, "An Error");
        WwiseProjectDatabaseLoggingUtils::ParseErrors();
    }
    
    SECTION("Expected Error with Multiple Occurences")
    {
        WwiseProjectDatabaseLoggingUtils::ResetErrors();
        WwiseProjectDatabaseLoggingUtils::AddExpectedError("An Error", 2);
        WWISE_DB_LOG(Error, "An Error");
        WWISE_DB_LOG(Error, "An Error");
        WwiseProjectDatabaseLoggingUtils::ParseErrors();
    }
}
