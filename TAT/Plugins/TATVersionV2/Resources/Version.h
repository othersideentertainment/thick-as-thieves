// (c) 2018-2019 OtherSide Entertainment, Inc. All rights reserved.
#pragma once

#include "Runtime/Launch/Resources/Version.h"
#include "BuildInfo.h"

#define TAT_BUILD_VERSION_STRING \
   VERSION_STRINGIFY(TAT_BUILD_VERSION_MAJOR) \
   VERSION_TEXT(".") \
   VERSION_STRINGIFY(TAT_BUILD_VERSION_MINOR) \
   VERSION_TEXT(".") \
   VERSION_STRINGIFY(TAT_BUILD_VCS_NUMBER)
