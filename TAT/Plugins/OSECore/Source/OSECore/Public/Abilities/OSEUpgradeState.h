// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"

// ose
#include "Abilities/OSESerializedTagMap.h"

#include "OSEUpgradeState.generated.h"

/// Map of Gameplay tags to int for storing progress of the player
/// TODO: better name that more clearly distinct from the front-end concept of upgrades
USTRUCT(BlueprintType)
struct OSECORE_API FUpgradeState : public FOSESerializedTagMap
{
   GENERATED_BODY()
};

// Not sure if this is redundant
template<>
struct TStructOpsTypeTraits< FUpgradeState > : public TStructOpsTypeTraitsBase2< FUpgradeState >
{
   enum
   {
      WithNetSerializer = true,
      WithNetSharedSerialization = true
   };
};
