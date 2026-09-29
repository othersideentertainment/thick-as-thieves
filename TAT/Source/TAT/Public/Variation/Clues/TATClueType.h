// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameplayTagContainer.h"

// an enum for use in routing between clue data and their compatible spawners
// Not expected to be used externally or in authored data
enum class ETATClueType : uint8
{
   None,
   Electrotype,
   ElectrotypeGlobal, //< could also be a subtype, but eh
   Readable,
   SpawnActor,
   TalkingDoor,
   NPC,
};

// opaque enum representing subtype
enum class ETATClueSubtype : uint8
{
   Default
};

template<typename OtherValue>
ETATClueSubtype AsClueSubtype(OtherValue val)
{
   return static_cast<ETATClueSubtype>(val);   
}

// A key that determines the set of clue spawners a clue is compatible with (must match exactly)
struct FTATClueBucketKey
{
   // hard compatibility enum that does not leak into data
   ETATClueType Type = ETATClueType::None;
   ETATClueSubtype Subtype = ETATClueSubtype::Default;
   // other user-defined constraints
   FGameplayTag PlacementTag;

   // (there will be padding either way, so might as well make it the right order)

   bool operator==(const FTATClueBucketKey& other) const
   {
      return Type == other.Type && Subtype == other.Subtype && PlacementTag == other.PlacementTag;
   }

   bool operator!=(const FTATClueBucketKey& other) const
   {
      return !(*this == other);
   }

   friend uint32 GetTypeHash(const FTATClueBucketKey& bucket)
   {
      return HashCombineFast(GetTypeHash(bucket.Type), HashCombineFast(GetTypeHash(bucket.Subtype), GetTypeHash(bucket.PlacementTag)));
   }
};
