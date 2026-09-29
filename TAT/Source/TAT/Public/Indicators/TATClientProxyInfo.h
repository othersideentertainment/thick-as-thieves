// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Iris/ReplicationState/IrisFastArraySerializer.h"

// tat
#include "Indicators/TATClientProxyActorTransform.h"

#include "TATClientProxyInfo.generated.h"

class APlayerController;

/// Represents a client proxy actor on the server.
/// Just the bare minimum needed to represent a client proxy.
/// Uses fast array serialization for replication.
/// See also: UTATThiefVisionSubsystem
USTRUCT()
struct FTATClientProxyInfo : public FFastArraySerializerItem
{
   GENERATED_BODY()

   /// An int32 should be enough for plenty of unique indicator IDs. In the pathological case (and with all positive IDs), this
   /// is roughly enough capacity to spawn 10,000 indicators per second for about 59 hours straight before running out.
   UPROPERTY()
   int32 UniqueId = INDEX_NONE;

   UPROPERTY()
   FGameplayTag IndicatorType;

   /// The server time this indicator was last refreshed.
   /// The lifetime of the indicator is assumed to be this value plus the indicator CDO's InitialLifeSpan.
   UPROPERTY()
   float LastRefreshServerWorldTime = 0.0f;

   UPROPERTY()
   FTATClientProxyActorTransform Transform;

   UPROPERTY()
   AActor* Instigator = nullptr;

   UPROPERTY()
   bool IsOutside = false;

   /// Generic user data for a client proxy that can be mapped to an enum or used as a small integer.
   /// This is useful for adding small variants to Thief Vision indicators without needing to set up new gameplay tags and data table rows.
   UPROPERTY()
   uint8 CustomData = 0;

   /// Check if this indicator is active and visible. If this returns false on a client, any cosmetic actor representing should be destroyed locally
   bool IsActive(float indicatorLifeSpan, float currentServerWorldTime) const;

   float GetRemainingLifeSpan(float indicatorLifeSpan, float currentServerWorldTime) const;

   /// Compares only fields that are allowed to update (eg. LastRefreshServerWorldTime) for equality
   bool ShouldUpdate(const FTATClientProxyInfo& newInfo) const;
};

USTRUCT()
struct FTATClientProxyInfoArray : public FIrisFastArraySerializer
{
   GENERATED_BODY()

   UPROPERTY()
   TArray<FTATClientProxyInfo> Items;

   TWeakObjectPtr<APlayerController> OwningLocalPlayerControllerWeak;

   void PreReplicatedRemove(const TArrayView<int32>& removedIndices, int32 finalSize);
   void PostReplicatedAdd(const TArrayView<int32>& addedIndices, int32 finalSize);
   void PostReplicatedChange(const TArrayView<int32>& changedIndices, int32 finalSize);

   bool NetDeltaSerialize(FNetDeltaSerializeInfo& deltaParams)
   {
      return FastArrayDeltaSerialize(Items, deltaParams, *this);
   }
};

template<>
struct TStructOpsTypeTraits<FTATClientProxyInfoArray> : public TStructOpsTypeTraitsBase2<FTATClientProxyInfoArray>
{
   enum
   {
      WithNetDeltaSerializer = true,
   };
};
