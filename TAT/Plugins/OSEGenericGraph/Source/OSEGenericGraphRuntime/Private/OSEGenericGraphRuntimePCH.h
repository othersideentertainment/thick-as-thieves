// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#pragma once

// #include "CoreUObject.h"
// #include "Engine.h"

// You should place include statements to your module's private header files here.  You only need to
// add includes for headers that are used in most of your module's source files though.
#include "IOSEGenericGraphRuntime.h"

#define LOG_INFO(FMT, ...) UE_LOG(OSEGenericGraphRuntime, Display, (FMT), ##__VA_ARGS__)
#define LOG_WARNING(FMT, ...) UE_LOG(OSEGenericGraphRuntime, Warning, (FMT), ##__VA_ARGS__)
#define LOG_ERROR(FMT, ...) UE_LOG(OSEGenericGraphRuntime, Error, (FMT), ##__VA_ARGS__)
