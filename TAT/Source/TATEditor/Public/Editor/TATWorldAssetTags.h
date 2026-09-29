// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "Containers/ContainersFwd.h"
#include "UObject/AssetRegistryTagsContext.h"
#include "UObject/NameTypes.h"
#include "UObject/Object.h"

class UWorld;

namespace TATWorldAssetTags
{
   extern const FName kLevelIsRandomized;

   // adds additional asset registry tags for the given world
   void AddWorldAssetTags(const UWorld* world, FAssetRegistryTagsContext context);

};
