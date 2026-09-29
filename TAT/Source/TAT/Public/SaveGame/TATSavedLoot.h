// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Loot/TATLootTypes.h"

// ue
#include "CoreMinimal.h"
#include "Misc/DateTime.h"

#include "TATSavedLoot.generated.h"

// NOTE: I don't love the surface area of this blueprint exposure.
//       It is mostly there for expedience in lieu of a richer c++ proxy for UI.

// TODO: Swap out TMap?

using FTATLootCountPair = TPair<FTATLootIdentifier, int32>;

USTRUCT(BlueprintType)
struct TAT_API FTATSavedLootStack
{
   GENERATED_BODY()

public:
   FTATSavedLootStack() = default;

   FTATSavedLootStack(FTATLootIdentifier lootId, int32 count)
   : LootIdentifier(lootId)
   , Count(count)
   {}


   // TODO: should this be a raw gameplaytag instead, for more of an explicit boundary?
   UPROPERTY(BlueprintReadOnly)
   FTATLootIdentifier LootIdentifier;

   UPROPERTY(BlueprintReadOnly)
   int32 Count = 0;

   void AppendToString(FString& string) const;

   bool operator==(const FTATLootIdentifier& id) const { return LootIdentifier == id; }
};


// A :shrug: intermediate struct so the code that touches the save doesn't touch the design data
struct FTATSavedLootAddRequest
{
   TMap<FTATLootIdentifier, int32> Stacks;

   void PopulateFrom(TConstArrayView<FTATLootIdentifier> loot);
   void AddCount(FTATLootIdentifier lootId, int32 count, UObject* contextObject);
};

struct FTATSavedLootRemoveRequest
{
   TArray<FTATLootInstanceId> Instances;
   TMap<FTATLootIdentifier, int32> Stacks;

   FORCEINLINE bool IsEmpty() const { return Instances.IsEmpty() && Stacks.IsEmpty(); }
};


USTRUCT(BlueprintType)
struct TAT_API FTATSavedLootInventory
{
   GENERATED_BODY()

public:
   void Init();
   void Add(const FTATSavedLootAddRequest& request);

   // Returns false if removal failed due to a loot item in request that doesn't exist in inventory
   // Stuffs loot items not found into 'outInvalidLoot', if non-null.
   bool Remove(const FTATSavedLootRemoveRequest& request);

   const TMap<FTATLootIdentifier, int32>& GetStacks() const { return Stacks; }

   void AppendToString(FString& string) const;

protected:
   // Returns true if all items in 'loot' param can be found within this inventory, false otherwise.
   bool ValidateLootForRemove(const FTATSavedLootRemoveRequest& lootToRemove) const;

   UPROPERTY(BlueprintReadOnly)
   TMap<FTATLootIdentifier, int32> Stacks;

   int32 _lastTransientId = 0;
};

// TODO: add BP wrappers here
UCLASS()
class TAT_API UTATSavedLootUtils : public UObject
{
   GENERATED_BODY()

public:

   /// Convenience method to convert to an array if more convenient
   /// Probably not long-term approach, since UI likely wants proxy UObjects
   UFUNCTION(BlueprintCallable, Category = "TAT|Loot|Saved")
   static void GetLootStacksAsArray(const FTATSavedLootInventory& inventory, TArray<FTATSavedLootStack>& outStacks);
};
