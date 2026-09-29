// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/SoftObjectPtr.h"

#include "TATLootTypes.generated.h"

class ATATLootActor;
class UGameplayEffect;
class UPaperSprite;
class USkeletalMesh;
class UStaticMesh;

//---------------------------------------------------------------------------------------------
// ETATLootType - indicates loot significance/rarity
//---------------------------------------------------------------------------------------------
UENUM(BlueprintType)
enum class ETATLootType : uint8
{
   None UMETA(Hidden),
   MinorLoot,
   MajorLoot,
   MAX UMETA(Hidden)
};

//---------------------------------------------------------------------------------------------
// ETATLootDropSettings - indicates the match settings for what kind of loot will drop on KO
//---------------------------------------------------------------------------------------------
UENUM(BlueprintType)
enum class ETATLootDropSettings : uint8
{
   DropAll = 0       UMETA(DisplayName = "Drop All Loot"),
   DropMajor = 1     UMETA(DisplayName = "Drop Only Major Loot"),
   KeepAll = 3       UMETA(DisplayName = "Keep All Loot")
};

//---------------------------------------------------------------------------------------------
// FTATLootIdentifier - unique loot identifier
//---------------------------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct TAT_API FTATLootIdentifier
{
   GENERATED_BODY()

   FTATLootIdentifier() = default;
   explicit FTATLootIdentifier(FGameplayTag tag) : LootTag(tag)
   {}

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (Categories="Loot"))
   FGameplayTag LootTag;

   // Returns true if this identifier corresponds to a valid loot item.
   bool IsValid() const { return LootTag.IsValid(); }

   // Clears this loot identifier, effectively making it "null"
   void Invalidate();

   FString ToString() const;

   FORCEINLINE bool operator==(const FTATLootIdentifier& other) const = default;
   
   bool NetSerialize(FArchive& ar, class UPackageMap* map, bool& outSuccess);
};

inline uint32 GetTypeHash(const FTATLootIdentifier& lootId)
{
   return GetTypeHash(lootId.LootTag);
}

template<>
struct TStructOpsTypeTraits< FTATLootIdentifier > : public TStructOpsTypeTraitsBase2< FTATLootIdentifier >
{
   enum
   {
      WithNetSerializer = true,
      WithNetSharedSerialization = true,
   };
};

//---------------------------------------------------------------------------------------
// FTATLootInstanceId - Used as an identifier for loot instances
//---------------------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct TAT_API FTATLootInstanceId
{
   GENERATED_BODY()

   FTATLootInstanceId() = default;
   explicit FTATLootInstanceId(int32 _Id) : Id(_Id) { }

   UPROPERTY(Transient)
   int32 Id = INDEX_NONE;

   bool IsValid() const { return Id != INDEX_NONE; }

   void Invalidate() { Id = INDEX_NONE; }

   bool operator==(const FTATLootInstanceId& other) const
   {
      return Id == other.Id;
   }

   bool operator!=(const FTATLootInstanceId& other) const
   {
      return !(*this == other);
   }
};

inline uint32 GetTypeHash(const FTATLootInstanceId& lootId)
{
   return GetTypeHash(lootId.Id);
}


//---------------------------------------------------------------------------------------
// FTATLootInstance - Runtime loot data
//---------------------------------------------------------------------------------------
USTRUCT(BlueprintType, meta = (HasNativeMake = "/Script/TAT.TATLootUtils.MakeLootInstance"))
struct TAT_API FTATLootInstance
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TAT Loot Instance")
   FTATLootIdentifier Identifier;

   UPROPERTY(Transient, BlueprintReadOnly, Category = "TAT Loot Instance")
   FTATLootInstanceId Id;

   // Returns true if this loot instance corresponds to a valid loot item.
   bool IsValid() const { return Identifier.IsValid() && Id.IsValid(); }

   // Clears this loot instance, effectively making it "null"
   void Invalidate();

   // Returns a debug representation of this loot instance for debugging or logging
   FString ToString() const;

   // Compares for equality all instance values except the identifier.
   // NB. If you add additional fields to this struct, you'll want to update this function
   // as well as FTATLootInfo::RequiresInstanceStorage to account for the new fields!
   FORCEINLINE bool InstanceDataEquals(const FTATLootInstance& other) const
   {
      return Id == other.Id;
   }

   FORCEINLINE bool operator==(const FTATLootInstance& other) const
   {
      return Identifier == other.Identifier && InstanceDataEquals(other);
   }

   FORCEINLINE bool operator!=(const FTATLootInstance& other) const { return !operator==(other); }

   // useful for a contains, as long as it doesn't create ambiguity
   bool operator==(const FTATLootIdentifier& id) const
   {
      return Identifier == id;
   }
};


/// Variant type that can store a loot identifier or a loot instance.
/// This is mostly just useful to simplify C++ APIs that need to handle both instances and identifiers.
struct FTATLootItemVariant
{
private:
   // Because a loot instance is just an identifier with extra data, we can avoid all the complexity that comes with TVariant or a C union type
   FTATLootInstance _data;
   bool _isInstance = false;
public:
   FTATLootItemVariant() = default;
   FTATLootItemVariant(const FTATLootItemVariant&) = default;
   FTATLootItemVariant& operator=(const FTATLootItemVariant&) = default;
   explicit FTATLootItemVariant(const FTATLootIdentifier& rhs) { _data.Identifier = rhs; _isInstance = false; }
   explicit FTATLootItemVariant(const FTATLootInstance& rhs) : _data(rhs), _isInstance(true) {}

   FORCEINLINE FTATLootIdentifier& Set(const FTATLootIdentifier& rhs) { _data.Invalidate(); _data.Identifier = rhs; _isInstance = false; return _data.Identifier; }
   FORCEINLINE FTATLootInstance& Set(const FTATLootInstance& rhs) { _data = rhs; _isInstance = true; return _data; }

   /// Does this loot item contain instance data?
   FORCEINLINE bool IsInstance() const { return _isInstance; }

   /// Gets the loot's identifier. Works whether or not we have instance data.
   FORCEINLINE FTATLootIdentifier& GetIdentifier() { return _data.Identifier; }
   FORCEINLINE const FTATLootIdentifier& GetIdentifier() const { return _data.Identifier; }

   /// Gets a reference to the loot's instance data. Will assert that this is an instance - check before calling!
   FORCEINLINE FTATLootInstance& GetInstanceRef() { check(IsInstance()); return _data; }
   FORCEINLINE const FTATLootInstance& GetInstanceRef() const { check(IsInstance()); return _data; }

   /// If this holds an instance, returns a copy of that instance. Otherwise returns a new instance with just the identifier in it.
   FORCEINLINE FTATLootInstance GetAsInstance() const { if (IsInstance()) { return _data; } FTATLootInstance result{}; result.Identifier = GetIdentifier(); return result; }

   /// Gets the loot's instance data, or nullptr if it does not hold instance data.
   FORCEINLINE FTATLootInstance* TryGetInstance() { return IsInstance() ? &_data : nullptr; }
   FORCEINLINE const FTATLootInstance* TryGetInstance() const { return IsInstance() ? &_data : nullptr; }

   FORCEINLINE FString ToString() const { return IsInstance() ? _data.ToString() : _data.Identifier.ToString(); }

   FORCEINLINE void Invalidate() { _data.Invalidate(); }

   FORCEINLINE bool IsValid() const { return IsInstance() ? _data.IsValid() : _data.Identifier.IsValid(); }
   FORCEINLINE explicit operator bool() const { return IsValid(); }

   FORCEINLINE bool operator==(const FTATLootIdentifier& rhs) const { return !IsInstance() && _data.Identifier == rhs; }
   FORCEINLINE bool operator==(const FTATLootInstance& rhs) const { return IsInstance() && _data == rhs; }
   FORCEINLINE bool operator==(const FTATLootItemVariant& rhs) const { return _isInstance ? (rhs._isInstance && _data == rhs._data) : (!rhs._isInstance && _data.Identifier == rhs._data.Identifier); }
};


//---------------------------------------------------------------------------------------
// FTATLootContainer - Stores loot identifiers as well as loot instances that can contain runtime data
//---------------------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct TAT_API FTATLootContainer
{
   GENERATED_BODY()

   // Creates a copy of this loot container, excluding the loot add/remove bindings
   FTATLootContainer Copy() const;

   FORCEINLINE const TArray<FTATLootIdentifier>& GetItems() const { return Items; }
   FORCEINLINE const TArray<FTATLootInstance>& GetInstances() const { return Instances; }

   FORCEINLINE int32 Num() const { return Items.Num() + Instances.Num(); }
   FORCEINLINE int32 NumInstances() const { return Instances.Num(); }
   FORCEINLINE int32 NumItems() const { return Items.Num(); }
   FORCEINLINE bool IsEmpty() const { return Items.IsEmpty() && Instances.IsEmpty(); }

   void Add(const FTATLootIdentifier& lootId);
   void Add(const FTATLootInstance& lootInst);
   void Add(const FTATLootItemVariant& lootItem);
   void Append(const TArray<FTATLootIdentifier>& items);
   void Append(const TArray<FTATLootInstance>& instances);
   void Append(const FTATLootContainer& rhs) { Append(rhs.Items); Append(rhs.Instances); }

   int32 RemoveItemSingle(const FTATLootIdentifier& lootId);
   void RemoveItemAtSwap(int32 lootIndex);
   void RemoveInstanceAt(int32 lootIndex);
   void RemoveInstanceAtSwap(int32 lootIndex);
   void ResetItems();
   void ResetInstances();
   void Reset() { ResetItems(); ResetInstances(); }

   int32 GetAmountOfLootType(const UObject* contextObject, ETATLootType lootType) const;

   // Gets a loot item or instance by container index.
   // Instances are first (0..NumInstances-1) followed by items (NumInstances..NumInstances+NumItems-1).
   FTATLootIdentifier Get(int32 containerIndex, FTATLootItemVariant* outItemVariant = nullptr) const;

   // Gets and removes a loot item or instance by container index.
   // Instances are first (0..NumInstances-1) followed by items (NumInstances..NumInstances+NumItems-1).
   FTATLootIdentifier RemoveAt(int32 containerIndex, FTATLootItemVariant* outItemVariant = nullptr);

   // Gets a loot item or instance by container index.
   // Instances are first (0..NumInstances-1) followed by items (NumInstances..NumInstances+NumItems-1).
   const FTATLootIdentifier& operator[](int32 containerIndex) const;

   // Calls a callback for each instance and item in this container.
   // Iterates over instances first, then items.
   // Usage: ForEach([](const FTATLootIdentifier& lootId, const FTATLootInstance* lootInst) {});
   template<typename Lambda>
   FORCEINLINE void ForEach(Lambda&& callback) const
   {
      for (const FTATLootInstance& lootInstance : Instances)
      {
         callback(lootInstance.Identifier, &lootInstance);
      }
      for (const FTATLootIdentifier& lootId : Items)
      {
         callback(lootId, nullptr);
      }
   }

protected:
   // Converts a "container index" (an index that refers to any item or instance in this container) to an index into one of the two arrays
   int32 _ContainerIndexToArrayIndex(int32 containerIndex, bool& outIsInstance) const;

   UPROPERTY(BlueprintReadOnly)
   TArray<FTATLootIdentifier> Items;
   UPROPERTY(BlueprintReadOnly)
   TArray<FTATLootInstance> Instances;
};


//---------------------------------------------------------------------------------------
// FTATCarriedLootMeshData - holds data describing how a major loot should attach 
// to the character's mesh
//---------------------------------------------------------------------------------------
USTRUCT()
struct TAT_API FTATCarriedLootMeshData
{
   GENERATED_BODY()

   FTATCarriedLootMeshData() {}

   // Mesh used to display when carried on the character's hand. Only used for major loot.
   UPROPERTY(EditAnywhere)
   TSoftObjectPtr<USkeletalMesh> ToolMesh = nullptr;
};


//---------------------------------------------------------------------------------------
// FTATLootStaticMeshData - holds data describing how a loot item should appear when shown in the world
// TODO: Delete if still unused
//---------------------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct TAT_API FTATLootStaticMeshData
{
   GENERATED_BODY()

   /// The static mesh asset that represents this loot type
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot Static Mesh Data")
   TSoftObjectPtr<UStaticMesh> StaticMesh;

   /// Material overrides (if any) that should be applied to the static mesh when displayed in the world
   UPROPERTY(EditAnywhere, Category = "Loot Static Mesh Data")
   TArray<TSoftObjectPtr<UMaterialInterface>> MaterialOverrides;

   /// Relative transform to apply to the static mesh when displayed in the world
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot Static Mesh Data")
   FTransform RelativeTransform;
};


//---------------------------------------------------------------------------------------
// FTATLootInfo - Data table row struct containing loot metadata
//---------------------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct TAT_API FTATLootInfo : public FTableRowBase
{
   GENERATED_BODY()

public:
   FTATLootInfo() {}

   // Type of loot
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   ETATLootType LootType = ETATLootType::MinorLoot;

   // Unique identifier
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FTATLootIdentifier LootIdentifier;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FText DisplayName;

   UPROPERTY(EditAnywhere, Category = Misc, meta = (InlineEditConditionToggle))
   bool UseSlotSizeOverride = false;

   // Number of slots this loot takes up in a loot inventory.
   // Only applies to loot inventory that that have capacity limits enabled (see: UTATLootInventoryComponent::InventorySlotMaxCapacity)
   UPROPERTY(EditAnywhere, Category = Misc, meta = (EditCondition = "UseSlotSizeOverride", UIMin = "1", ClampMin = "1"))
   int32 SlotSizeOverride = 1;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TSoftObjectPtr<UPaperSprite> DisplaySprite = nullptr;

   // Metadata describing how the loot mesh should attach to the character when carried. Only used for major loot.
   UPROPERTY(EditAnywhere, Meta = (EditCondition = "IsLargeCarry", EditConditionHides))
   FTATCarriedLootMeshData CarriedLootMeshData;

   // The static mesh asset that represents this loot type
   // TODO: Delete if unused
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FTATLootStaticMeshData LootStaticMesh;

   // Optional override for the sound played when picked up. If unassigned, a default sound (specified in TATLootInventory) will be played.
   UPROPERTY(EditAnywhere, Category="Misc", Meta = (Categories = "GameplayCue"))
   FGameplayTag PickupGameplayCueOverride;

   // Metagame money value when successfully stolen
   UPROPERTY(EditAnywhere)
   int Value = 0;

   // World actor representation
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TSoftClassPtr<ATATLootActor> ActorClass;

   // Converts to money at the end of a match (for now)
   // TODO: Likely will want to be its own loot type once it converts to cash on pickup (to reduce inventory pressure), but this has unresolved design questions, so punting on that for now
   UPROPERTY(EditAnywhere, Meta = (EditCondition = "LootType == ETATLootType::MinorLoot", EditConditionHides))
   bool AutoConvertToMoney = false;

   // Whether it is always held in hand when picked up
   // see https://otherside.atlassian.net/wiki/spaces/TVT/pages/3279421447/Large+Carry+Items
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   bool IsLargeCarry = false;

   // Whether this loot is used in the context of a quest
   // TODO: (may or may not be loot type in future)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   bool IsQuestRelated = false;

   // The type of location it would spawn for quests
   UPROPERTY(EditAnywhere, Meta = (EditCondition = "IsQuestRelated", EditConditionHides, Categories="QuestLocation"))
   FGameplayTag QuestLocationTag;

   // Clue set associated with this loot (largely for major loot)
   // CLUE-WIP: Will definitely change how clue assets work
   UPROPERTY(EditDefaultsOnly, Category = "Clues", meta = (AllowedClasses="/Script/TAT.TATClueSet", EditCondition = "LootType == ETATLootType::MajorLoot", EditConditionHides))
   FSoftObjectPath ClueSet;

   UPROPERTY(EditAnywhere)
   float TimeTakenToSteal = 1.f;

   // Gameplay effect applied while the loot is held in inventory
   UPROPERTY(EditAnywhere, Category="Effects")
   TSoftClassPtr<UGameplayEffect> CarriedGameplayEffect;

public:
   /// Gets the size in slots that this item takes up in a loot inventory component
   int32 GetSlotSize() const;

   /// Checks if this loot type has associated runtime data that needs to be stored in an FTATLootInstance
   bool RequiresInstanceStorage() const;

   /// Constructs a default loot instance for this loot type.
   FTATLootInstance CreateDefaultInstance(const UObject* contextObj) const;

   bool IsValid() const { return LootIdentifier.IsValid(); }
};


//---------------------------------------------------------------------------------------
// FTATLootMetadataBP - Subset of FTATLootInfo that is suitable for passing around in Blueprints without copying a lot of extra data
//---------------------------------------------------------------------------------------
USTRUCT(BlueprintType, DisplayName = "TAT Loot Metadata")
struct FTATLootMetadataBP
{
   GENERATED_BODY()

   FTATLootMetadataBP() = default;
   explicit FTATLootMetadataBP(const FTATLootInfo& lootInfo);

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loot Metadata")
   FTATLootIdentifier LootIdentifier;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loot Metadata")
   ETATLootType LootType = ETATLootType::None;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loot Metadata")
   int32 SlotSize = 1;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loot Metadata")
   FText DisplayName;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loot Metadata")
   TSoftObjectPtr<UPaperSprite> DisplaySprite;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loot Metadata")
   int LootValue = 0;
};


/// Describes for loot actors how they're placed: are where they initially spawned, have they been dropped by a player/guard,
// or have they (in the case of major loot) been returned to a return location
UENUM(BlueprintType)
enum class ETATLootPlacementState : uint8
{
   InitialSpawn,
   Dropped,
   Returned
};
