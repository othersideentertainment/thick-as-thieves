// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "TATInventoryTypes.generated.h"

// Equippable items
// Items in toolbelt are tool-ified. There can only be one stack of a given item
// in the toolbelt at a time.
// 
// Priority of adding an equippable:
// a) existing stack in toolbelt
// b) existing stack in backpack
// c) new stack in toolbelt
// d) new stack in backpack

UENUM(BlueprintType)
enum class EInventoryType : uint8
{
   Backpack,
   QuestItems,
   Toolbelt
};

/// A struct for a non-persistent identifier for an item stack.
/// There is no guarantee that this id is maintained if the stack if moved to a different context
USTRUCT(BlueprintType)
struct TAT_API FInventoryStackId
{
   GENERATED_BODY()

public:

   FInventoryStackId() : _value(-1) {}

   explicit FInventoryStackId(int32 value)
      : _value(value)
   {}

   FORCEINLINE bool operator==(FInventoryStackId const& other) const
   {
      return _value == other._value;
   }

   FORCEINLINE bool operator!=(FInventoryStackId const& other) const
   {
      return _value != other._value;
   }

   FORCEINLINE bool operator<(FInventoryStackId const& other) const
   {
      return _value < other._value;
   }
   
   FORCEINLINE bool IsValid() const
   {
      return _value > 0;
   }

   FORCEINLINE friend uint32 GetTypeHash(const FInventoryStackId& id)
   {
      return ::GetTypeHash(id._value);
   }

   int32 GetValue() const { return _value; }

   static const FInventoryStackId Invalid;

private:
   UPROPERTY()
   int32 _value;
};

template<>
struct TStructOpsTypeTraits<FInventoryStackId> : public TStructOpsTypeTraitsBase2<FInventoryStackId>
{
   enum
   {
      WithIdenticalViaEquality = true,
   };
};

USTRUCT(BlueprintType)
struct TAT_API FTATInventorySize
{
   GENERATED_BODY()

public:
   // probably discourage blueprint from reading from fields directly in favor of method (if needed at all)?

   UPROPERTY(EditDefaultsOnly)
   uint8 BackpackSize = 10;

   UPROPERTY(EditDefaultsOnly)
   uint8 ToolbeltSize = 1;

   int32 GetSizeForBucket(EInventoryType bucket) const;
};
