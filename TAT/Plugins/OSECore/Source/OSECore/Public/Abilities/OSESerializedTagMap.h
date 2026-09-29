// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "OSESerializedTagMap.generated.h"

/// Map of Gameplay tags to int that can be net-serialized
/// (not required for normal, not-net, serialization)
USTRUCT(BlueprintType)
struct OSECORE_API FOSESerializedTagMap
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadOnly, NotReplicated)
   TMap<FGameplayTag, int32> Values;

   bool HasTag(FGameplayTag tag) const { return Values.Contains(tag); }
   int32 GetValue(FGameplayTag tag, int32 fallback = 0) const
   {
      const int32* result = Values.Find(tag);
      return result ? *result : fallback;
   }


   void MergeNumerically(const FOSESerializedTagMap& other);

   bool NetSerialize(FArchive& ar, class UPackageMap* map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits< FOSESerializedTagMap > : public TStructOpsTypeTraitsBase2< FOSESerializedTagMap >
{
   enum
   {
      WithNetSerializer = true,
      WithNetSharedSerialization = true
   };
};
