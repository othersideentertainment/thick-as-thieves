// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Editor/TATWorldAssetTags.h"

// tat
#include "GameFramework/TATWorldSettings.h"

// ose
#include "OSECommon.h"

// ue5
#include "Engine/World.h"

const FName TATWorldAssetTags::kLevelIsRandomized(TEXT("LevelIsRandomized"));

void TATWorldAssetTags::AddWorldAssetTags(const UWorld* world, FAssetRegistryTagsContext context)
{
   using FAssetRegistryTag = UObject::FAssetRegistryTag;
   if (const ATATWorldSettings* worldSettings = Cast<ATATWorldSettings>(world->GetWorldSettings()))
   {
      static const FName kMapType(TEXT("MapType"));
      context.AddTag(FAssetRegistryTag(kMapType, UOSECommon::UnqualifiedEnumToString(worldSettings->MapType), FAssetRegistryTag::TT_Alphabetical));

      const bool isRandomized = worldSettings->SpawnData != nullptr;
      context.AddTag(FAssetRegistryTag(kLevelIsRandomized, isRandomized ? TEXT("True") : TEXT("False"), FAssetRegistryTag::TT_Alphabetical));
   }
}
