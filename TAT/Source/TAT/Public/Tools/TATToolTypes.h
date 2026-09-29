// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "WorldActors/TATToolWorldActorData.h"

// ue5
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"

#include "TATToolTypes.generated.h"

class UTATToolComponent;
class UPaperSprite;
class UInputAction;

UENUM()
enum class ETATToolAmmoType : uint8
{
   Finite,
   Infinite
};

/// Used to store and apply parameters for the actor that a tool wants to spawn as part of its gameplay-related effects
/// NOTE: Some tools may end up wanting other modes of effecting the world. It's likely that we will need alternative
/// structs to handle those cases, and some way of unifying them
USTRUCT(BlueprintType)
struct TAT_API FTATGearWorldActorParameters
{
   GENERATED_BODY()
public:

   /// The class to spawn in to create the tool's world actor
   UPROPERTY(Transient, BlueprintReadOnly)
   TSoftClassPtr<AActor> WorldActorClass;

   /// The class of tool that created this 
   UPROPERTY(Transient, BlueprintReadOnly)
   TSubclassOf<UTATToolComponent> ParentToolClass;

   /// Optional data to be used by the world actor
   UPROPERTY(Transient, BlueprintReadOnly)
   FTATToolWorldActorData WorldActorData;

   void Reset()
   {
      ParentToolClass = nullptr;
      WorldActorClass = nullptr;
      WorldActorData.OptionalIntValue = 0;
   }
};

/// Describes when a tool's deployable can be picked up
UENUM()
enum class ETATDeployablePickupCapability
{
   /// Deployable can never be picked back up once deployed
   Never,

   /// Deployable can be picked up until it activates for the first time
   BeforeActivation,

   /// Deployable can always be picked up
   Always
};

/// Represents metadata for a piece of gear: meant to be loaded separately
USTRUCT(BlueprintType)
struct TAT_API FTATGearMetadata
{
   GENERATED_BODY()
public:

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FText ToolName;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FText ToolDescription;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayThumbnail = "true"))
   TSoftObjectPtr<UPaperSprite> IconSprite;

   /// Custom input action that toggles this tool. Used for UI display.
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TSoftObjectPtr<UInputAction> InputAction;
};

USTRUCT(BlueprintType)
struct FTATToolUpgrade
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Upgrade")
   FGameplayTag UpgradeTag;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Upgrade", Meta = (UIMin = 1, ClampMin = 1))
   int32 UpgradeLevel = 1;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Upgrade")
   TSoftClassPtr<UTATToolComponent> UpgradeToolClass;

   bool IsValid() const { return UpgradeTag.IsValid() && !UpgradeToolClass.IsNull(); }
};

USTRUCT(BlueprintType)
struct TAT_API FTATGearMetadataTableRow : public FTableRowBase
{
   GENERATED_BODY()
public:

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (Categories = "Tool.Type"))
   FGameplayTag ToolID;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FTATGearMetadata Metadata;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TSoftClassPtr<UTATToolComponent> ToolClass;

   /// Category the tool is in. Defines which loadout slots the tool can be placed in.
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (Categories = "Loadout.Slot"))
   FGameplayTag ToolLoadoutSlotCategory;

   /// A character with this upgrade tag and level will get this tool instead of the normal tool class.
   /// Useful for less granular upgrades (eg. an upgrade that completely replaces another tool)
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FTATToolUpgrade ToolUpgrade;

#if WITH_EDITOR
   // NB: The data table itself also has a OnDataTableChanged delegate, however when using FDataTableRowHandle, we only have
   // a const pointer to the data table, but can get a mutable pointer to the individual row
   DECLARE_MULTICAST_DELEGATE(FOnMetadataDataTableChanged);
   FOnMetadataDataTableChanged OnMetadataDataTableChanged;

   virtual void OnDataTableChanged(const UDataTable* dataTable, const FName rowName) override;
#endif
};

USTRUCT(BlueprintType)
struct FTATWorldActorBoxFillAdjustedTransform
{
   GENERATED_BODY()
public:
   /// The total height that we can occupy
   UPROPERTY(BlueprintReadOnly)
   float Height = 0.0f;

   /// The half extents (think radius) that we can occupy in our two horizontal axes
   UPROPERTY(BlueprintReadOnly)
   FVector2D HalfExtents2D = FVector2D::ZeroVector;

   /// The centre of our extents in world space
   UPROPERTY(BlueprintReadOnly)
   FVector WorldPosition = FVector::ZeroVector;

   /// Our new rotation in world space, facing the shortest axis
   UPROPERTY(BlueprintReadOnly)
   FRotator WorldRotation = FRotator::ZeroRotator;

   // C++20 allows default comparison operators, until then manually specify
   inline bool operator==(const FTATWorldActorBoxFillAdjustedTransform& other) const
   {
      return Height == other.Height
         && HalfExtents2D == other.HalfExtents2D
         && WorldPosition == other.WorldPosition
         && WorldRotation == other.WorldRotation;
   }

   inline bool operator!=(const FTATWorldActorBoxFillAdjustedTransform& other) const
   {
      return !(*this == other);
   }
};

USTRUCT(BlueprintType)
struct FTATWorldActorBoxFillExtentConstraints
{
   GENERATED_BODY()
public:
   /// What is the maximum height we can take up
   UPROPERTY(BlueprintReadWrite)
   float MaxHeight = 100.0f;

   /// What are the maximum dimensions we can occupy in the XY plane
   UPROPERTY(BlueprintReadWrite)
   float MaxHalfExtents2D = 250.0f;

   /// What profile should we use for traces when computing how much space we can take up
   UPROPERTY(BlueprintReadWrite)
   FCollisionProfileName TraceProfile;
};

