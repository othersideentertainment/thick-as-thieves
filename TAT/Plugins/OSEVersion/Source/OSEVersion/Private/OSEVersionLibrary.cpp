// (c) OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "OSEVersionLibrary.h"
#include "Version.h"

FString UOSEVersionLibrary::GetBuildVersionString() { return FString(OSE_BUILD_VERSION_STRING); }

FString UOSEVersionLibrary::GetBuildDate() { return FString(OSE_BUILD_DATE); }

int32 UOSEVersionLibrary::GetBuildVersionMajor() { return OSE_BUILD_VERSION_MAJOR; }

int32 UOSEVersionLibrary::GetBuildVersionMinor() { return OSE_BUILD_VERSION_MINOR; }

int32 UOSEVersionLibrary::GetBuildNumber() { return OSE_BUILD_NUMBER; }

int32 UOSEVersionLibrary::GetBuildChangelistNumber() { return OSE_BUILD_CL; }

FString UOSEVersionLibrary::GetBuildBranch() { return FString(VERSION_TEXT(OSE_BUILD_BRANCH)); }

FString UOSEVersionLibrary::GetBuildConfiguration() { return FString(VERSION_TEXT(OSE_BUILD_CONFIGURATION)); }
