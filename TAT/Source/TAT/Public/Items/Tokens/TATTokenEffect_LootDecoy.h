// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATTokenEffect.h"
#include "Loot/TATLootTypes.h"

#include "TATTokenEffect_LootDecoy.generated.h"

// Will replace the calling card for a specific loot type when it is taken
USTRUCT(DisplayName="Loot Swap Decoy")
struct FTATTokenEffect_LootDecoy : public FTATTokenEffect
{
   GENERATED_BODY()

   // The loot type to be a decoy for (when taken)
   UPROPERTY(EditAnywhere)
   FTATLootIdentifier Loot;

   // The actor that will get dropped
   // Must implement TATLootDecoyActorInterface to filter options
   UPROPERTY(EditAnywhere, meta = (MustImplement="/Script/TAT.TATLootDecoyActorInterface"))
   TSoftClassPtr<AActor> DecoyActorClass;

   static bool TryDropDecoy(const AActor* owningActor, const FTATLootIdentifier& lootType, const FTransform& transform);

#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void (const FText&)> reportError) const override;
#endif
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UTATLootDecoyActorInterface : public UInterface
{
   GENERATED_BODY()
};

/// A marker interface for actors that are used as loot decoys, for filtering by
class TAT_API ITATLootDecoyActorInterface
{
   GENERATED_BODY()
};

