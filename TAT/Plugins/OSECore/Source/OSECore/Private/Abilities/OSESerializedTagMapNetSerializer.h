// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameplayTagContainer.h"
#include "Iris/Serialization/NetSerializer.h"

#include "OSESerializedTagMapNetSerializer.generated.h"

// An implementation detail used by FOSESerializedTagMapNetSerializer
USTRUCT()
struct FOSESerializedTagMap_ProxyEntry
{
   GENERATED_BODY()

   UPROPERTY()
   FGameplayTag Tag;

   UPROPERTY()
   int Count = 0;
};

// An implementation detail used by FOSESerializedTagMapNetSerializer
USTRUCT()
struct FOSESerializedTagMap_Proxy
{
   GENERATED_BODY()

   UPROPERTY()
   TArray<FOSESerializedTagMap_ProxyEntry> Entries;
};

USTRUCT()
struct FOSESerializedTagMapNetSerializerConfig : public FNetSerializerConfig
{
   GENERATED_BODY()
};

namespace UE::Net
{
   UE_NET_DECLARE_SERIALIZER(FOSESerializedTagMapNetSerializer, OSECORE_API);
}
