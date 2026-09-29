// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Loot/TATLootTypes.h"

// ue
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TATLootUtils.generated.h"

class ATATLootActor;
struct FTATDamageWithType;

UCLASS()
class TAT_API UTATLootUtils : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   /// Checks if a loot identifier contains a valid loot tag with a valid data table entry.
   UFUNCTION(BlueprintPure, Category = "TAT|Loot", Meta = (WorldContext = "worldContextObject"))
   static bool IsLootIdentifierValid(const UObject* worldContextObject, FTATLootIdentifier lootIdentifier);

   /// Checks if two loot identifiers are equal
   UFUNCTION(BlueprintPure, Category = "TAT|Loot", Meta = (DisplayName = "Equal (TATLootIdentifier)", CompactNodeTitle = "==", ScriptOperator = "==", BlueprintThreadSafe))
   static inline bool EqualEqual_LootIdentifier(FTATLootIdentifier identA, FTATLootIdentifier identB) { return identA == identB; }

   /// Checks if a loot instance contains a valid loot tag with a valid data table entry.
   UFUNCTION(BlueprintPure, Category = "TAT|Loot", Meta = (WorldContext = "worldContextObject"))
   static bool IsLootInstanceValid(const UObject* worldContextObject, const FTATLootInstance& lootInstance);

   /// Finds loot metadata from a loot identifier.
   UFUNCTION(BlueprintCallable, Category = "TAT|Loot", Meta = (WorldContext = "worldContextObject", ExpandBoolAsExecs = "ReturnValue"))
   static bool FindLootMetadata(const UObject* worldContextObject, FTATLootIdentifier lootIdentifier, FTATLootMetadataBP& lootMetadata);

   /// Finds loot info from a loot identifier.
   UFUNCTION(BlueprintPure, Category = "TAT|Loot", Meta = (WorldContext = "worldContextObject"))
   static FTATLootInfo FindLootInfo(const UObject* worldContextObject, FTATLootIdentifier lootIdentifier);

   /// Gets the loot type for a given loot identifier.
   UFUNCTION(BlueprintPure, Category = "TAT|Loot", Meta = (WorldContext = "worldContextObject"))
   static ETATLootType GetLootType(const UObject* worldContextObject, FTATLootIdentifier lootIdentifier);

   static int CountLootOfType(const UObject* worldContextObject, TConstArrayView<FTATLootIdentifier> lootIdentifiers, ETATLootType lootType);
   static int CountValueLootOfType(const UObject* worldContextObject, TConstArrayView<FTATLootIdentifier> lootIdentifiers, ETATLootType lootType);

   DECLARE_DYNAMIC_DELEGATE_FourParams(FOnLoadedLootStaticMeshData, FTATLootIdentifier, lootIdentifier, UStaticMesh*, staticMesh, const TArray<UMaterialInterface*>&, materialOverrides, FTransform, relativeTransform);
   using FOnLoadedLootStaticMeshDataFn = TFunction<void(FTATLootIdentifier, UStaticMesh*, const TArray<UMaterialInterface*>&, const FTransform&)>;

   /// Async-loads the loot static mesh params for a loot type.
   /// Returns true if the onLoadComplete event will be fired (this really only returns false if the loot identifier is not valid)
   static bool GetLootStaticMeshDataAsyncNative(const UObject* worldContextObject, FTATLootIdentifier lootIdentifier, const FOnLoadedLootStaticMeshDataFn& onLoadComplete);

   /// Async-loads the loot static mesh params for a loot type.
   /// Returns true if the onLoadComplete event will be fired (this really only returns false if the loot identifier is not valid)
   UFUNCTION(BlueprintCallable, Category = "TAT|Loot", Meta = (WorldContext = "worldContextObject"))
   static bool GetLootStaticMeshDataAsync(const UObject* worldContextObject, FTATLootIdentifier lootIdentifier, FOnLoadedLootStaticMeshData onLoadComplete);

   /// Gets the base (metadata) value for loot based on a loot identifier.
   UFUNCTION(BlueprintPure, Category = "TAT|Loot", Meta = (WorldContext = "worldContextObject"))
   static int32 GetLootBaseValue(const UObject* worldContextObject, FTATLootIdentifier lootIdentifier);

   /// Gets the base (metadata) value for a loot instance.
   UFUNCTION(BlueprintPure, Category = "TAT|Loot", Meta = (WorldContext = "worldContextObject"))
   static int32 GetLootInstanceBaseValue(const UObject* worldContextObject, const FTATLootInstance& lootInstance);

   /// Creates a NEW loot instance, set up correctly for the specified type.
   UFUNCTION(BlueprintPure, Category = "TAT|Loot", Meta = (WorldContext = "worldContextObject"))
   static FTATLootInstance MakeLootInstance(const UObject* worldContextObject, FTATLootIdentifier lootIdentifier);

   /// Checks if a given loot type requires an FTATLootInstance when storing or moving it.
   UFUNCTION(BlueprintPure, Category = "TAT|Loot", Meta = (WorldContext = "worldContextObject"))
   static bool RequiresInstanceStorage(const UObject* worldContextObject, FTATLootIdentifier lootIdentifier);

   /// Resets a loot instance to not refer to any type of loot.
   /// Use this after transferring ownership to avoid accidently duplicating loot instances.
   UFUNCTION(BlueprintCallable, Category = "TAT|Loot")
   static inline void InvalidateLootInstance(UPARAM(ref) FTATLootInstance& lootInstance) { lootInstance.Invalidate(); }


   static FVector CalcLootPinataSpawnLocation(const FVector& pinataLocation, int32 itemIndex, int32 numItems, const FFloatInterval& radiusRange);

   /// Helper for spawning a loot actor in the world.
   /// It's safe for droppingActor to be null if there isn't one.
   static ATATLootActor* SpawnLootActor(
      UWorld* world,
      const AActor* droppingActor,
      TSubclassOf<ATATLootActor> lootClass,
      const FTATLootItemVariant& lootItem,
      const FVector& suggestedDropLocation);

   /// Helper for spawning a loot actor in the world with a soft loot actor class reference.
   /// It's safe for droppingActor to be null if there isn't one.
   static void SpawnLootActorAsync(
      UWorld* world,
      const AActor* droppingActor,
      const TSoftClassPtr<ATATLootActor>& lootClass,
      const FTATLootItemVariant& lootItem,
      const FVector& suggestedDropLocation);

};

