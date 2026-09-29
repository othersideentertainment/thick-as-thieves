// (c) OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
struct OSEBUGREPORTER_API FOSEBugData
{
    FString Description;
    FString Summary;
    FString BuildVersion;
    FString BuildConfiguration;
    FString Platform;
    FString GPU;
    FString MapName;
    FString DirectObjectData;
    FString RadiusObjectData;
    float WorldTime;
    FVector WorldPosition;
};
