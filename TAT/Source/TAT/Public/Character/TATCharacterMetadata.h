// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "CharacterCustomization/TATCharacterLoadout.h"
#include "SaveGame/TATCharacterDataContext.h"

// ue4
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TATCharacterMetadata.generated.h"

class ATATCharacter;
class UAnimSequence;
class USkeletalMesh;
class UTexture2D;
class UTATUpgradeGraph;
enum class ETATCharacter : uint8;
struct FTATCharacterSaveId;
struct FTATCharacterDataContext;

//-------------------------------------------------------------------------------
/// Data struct with user-facing information on one of a character's abilities.
//-------------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct TAT_API FTATCharacterAbilityMetadata
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FText AbilityName;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FText AbilityDescription;
};

USTRUCT(BlueprintType)
struct FTATCharacterAvailableLoadout
{
   GENERATED_BODY()

   /// The maximum number of tools a player can select to be in their loadout from ToolIds
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout", meta = (UIMin = 0, ClampMin = 0))
   int32 NumToolSlots = 5;

   /// What tools this character type is allowed to use
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Character Loadout Metadata", Meta = (Categories = "Tool.Type"))
   TArray<FGameplayTag> ToolIds;
};

/// Defines a gameplay tag that is valid for placement in a loadout
USTRUCT(BlueprintType)
struct TAT_API FTATCharacterAvailableLoadoutEntry
{
   GENERATED_BODY()

   /// Tool or ability tag for this entry
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Character Available Loadout Entry")
   FGameplayTag LoadoutTag;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Character Available Loadout Entry", meta = (Categories = "Upgrade"))
   FGameplayTag UpgradeTag;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Character Available Loadout Entry")
   int32 UpgradeLevel = 1;

   bool IsValid() const { return LoadoutTag.IsValid(); }

   FString ToString() const { return LoadoutTag.ToString(); }

   bool MeetsUpgradeRequirement(const FTATCharacterDataContext& context) const;
};

USTRUCT(BlueprintType)
struct TAT_API FTATCharacterLoadoutBlock
{
   GENERATED_BODY()

   /// The loadout slot category for this block of slots
   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Loadout", meta = (Categories = "Loadout.Slot"))
   FGameplayTag SlotCategory;

   /// The base number of loadout slots that a player can assign to loadout entries with this category
   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Loadout", meta = (UIMin = 0, ClampMin = 0))
   int32 SlotCountBase = 1;

   /// Characters with this upgrade tag will gain additional loadout slots using the slot count upgrade curve
   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Loadout", meta = (Categories = "Upgrade"))
   FGameplayTag SlotCountUpgradeTag;

   /// If SlotCountUpgradeTag is specified, the upgrade level will be used to pick the value in this curve to pick how many additional loadout slots to give
   /// the character (this value will be added to SlotCountBase).
   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Loadout")
   FScalableFloat SlotCountUpgradeCurve;
};

/// A loadout category and the loadout index range that is valid for that category
USTRUCT(BlueprintType)
struct FTATCharacterLoadoutCategorySpan
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Character Loadout Category Span", meta = (Categories = "Loadout.Slot"))
   FGameplayTag Category;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Character Loadout Category Span")
   int32 StartIndex = 0;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Character Loadout Category Span")
   int32 SlotCount = 0;

   FTATCharacterLoadoutCategorySpan() = default;

   FTATCharacterLoadoutCategorySpan(FGameplayTag category, int32 startIndex, int32 slotCount)
      : Category(category)
      , StartIndex(startIndex)
      , SlotCount(slotCount)
   {
   }

   FORCEINLINE bool ContainsIndex(int32 index) const
   {
      return index >= 0 && SlotCount > 0 && index >= StartIndex && index < StartIndex + SlotCount;
   }
};

USTRUCT(BlueprintType)
struct TAT_API FTATCharacterLoadoutConfig
{
   GENERATED_BODY()

   /// What loadout tags this character type is allowed to include in their loadout
   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Loadout", meta = (TitleProperty = "LoadoutTag"))
   TArray<FTATCharacterAvailableLoadoutEntry> AvailableLoadout;

   /// Default loadout for this character.
   /// If the character has a slot limit that is smaller than the size of this array, they will only get the first X items from this array when
   /// resetting to the default loadout (where X is the current slot limit)
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loadout", meta = (TitleProperty = "LoadoutTag"))
   TArray<FTATCharacterLoadoutEntry> DefaultLoadout;

   /// What items from this loadout should appear on the character select screen for this character?
   /// This should be a list of important tools/abilities unique to this character.
   /// NB. This value is purely for UI display purposes and has no gameplay effect
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loadout")
   TArray<FGameplayTag> FeaturedOnCharacterSelectScreen;

   /// Blocks of loadout slots, with each block being constrained to a tool category
   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Loadout", meta = (TitleProperty = "{SlotCountBase} Slots - {SlotCategory}"))
   TArray<FTATCharacterLoadoutBlock> LoadoutSlotBlocks;

   const FTATCharacterAvailableLoadoutEntry* FindAvailableLoadoutEntryByTag(FGameplayTag loadoutTag) const;

   /// Finds a loadout block with a particular category
   const FTATCharacterLoadoutBlock* FindLoadoutBlock(FGameplayTag category) const;

   /// Checks if a given loadout is valid for this loadout config
   bool IsValidLoadout(const FTATCharacterDataContext& context, const FTATCharacterLoadout& loadout) const;

   /// Gets a character's total number of loadout slots (in all categories)
   int32 GetLoadoutTotalSlotCount(const FTATCharacterDataContext& context) const;

   /// Gets a character's number of available loadout slots for a given category
   int32 GetLoadoutSlotCountInCategory(const FTATCharacterDataContext& context, FGameplayTag loadoutCategory) const;

   /// Gets all categories and the slot indices that they span.
   /// Returns the total number of slots (the sum of the SlotCount field in the outCategorySpans array).
   template<typename Alloc = FDefaultAllocator>
   int32 GetTotalSlotCountAndCategorySpans(const FTATCharacterDataContext& context, TArray<FTATCharacterLoadoutCategorySpan, Alloc>& outCategorySpans) const
   {
      int32 totalSlotCount = 0;
      for (const FTATCharacterLoadoutBlock& block : LoadoutSlotBlocks)
      {
         outCategorySpans.Emplace(block.SlotCategory, totalSlotCount, GetLoadoutSlotCountInCategory(context, block.SlotCategory));
         totalSlotCount += outCategorySpans.Last().SlotCount;
      }
      return totalSlotCount;
   }

   /// Checks if a loadout entry with a given category is valid for a loadout index.
   /// Takes an array of category spans (you can build this array with GetTotalSlotCountAndCategorySpans)
   static bool IsCategoryValidForSlot(TConstArrayView<FTATCharacterLoadoutCategorySpan> categorySpans, FGameplayTag category, int32 loadoutIndex);

#if WITH_EDITOR
   void IsDataValid(FDataValidationContext& context, const FText& errorPrefix) const;
#endif
};

USTRUCT()
struct TAT_API FTATCharacterMetadata
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, Category = "Description")
   FText LocalizedName;

   UPROPERTY(EditAnywhere, Category = "Description")
   FText LocalizedClassName;

   UPROPERTY(EditAnywhere, Category = "Description")
   FText LocalizedNickname;

   UPROPERTY(EditAnywhere, Category = "Description")
   FText LocalizedDescription;

   /// Upgrade graph to use for this character
   UPROPERTY(EditAnywhere, Category = "Enhancements")
   TSoftObjectPtr<UTATUpgradeGraph> UpgradeGraph;

   /// Upgrade tags that are always added for this character.
   /// This is intended to allow per-character-type baseline enhancements to tools and abilities that are shared by all characters (like the blackjack)
   UPROPERTY(EditAnywhere, Category = "Enhancements", Meta = (Categories = "Upgrade", ForceInlineRow, ClampMin = 1, UIMin = 1))
   TMap<FGameplayTag, int32> IntrinsicUpgrades;

   /// All possible abilities this character can use in their loadout
   UPROPERTY(EditAnywhere, Category = "Loadout", Meta = (Categories = "Ability.Type"))
   FTATCharacterLoadoutConfig AbilityLoadoutConfig;

   /// All possible tools this character can use in their loadout
   UPROPERTY(EditAnywhere, Category = "Loadout", Meta = (Categories = "Tool.Type"))
   FTATCharacterLoadoutConfig ToolLoadoutConfig;

   /// All possible outfits this character can use in their loadout
   UPROPERTY(EditAnywhere, Category = "Loadout", Meta = (Categories = "Outfit.Type"))
   FTATCharacterLoadoutConfig OutfitLoadoutConfig;

   // User-facing information about each unique ability available to this character.
   // DEPRECATED: Use AvailableLoadout, DefaultGearLoadout, CharacterSelectScreenGear, or UTATSaveGame::GetGearLoadout() instead (depending on the use-case).
   UPROPERTY(EditAnywhere, DisplayName = "StartingGearMetadata_DEPRECATED", Category = "Gear")
   TArray<FDataTableRowHandle> StartingGearMetadata;

   // User-facing image to symbolize the character's identity on glyphs as well as calling cards, stashes, etc
   UPROPERTY(EditAnywhere, Category = "Art")
   TSoftObjectPtr<UTexture2D> SymbolTexture;

   UPROPERTY(EditAnywhere, Category = "Abilities")
   TArray<FTATCharacterAbilityMetadata> LocalizedAbilityMetadata;

   UPROPERTY(EditAnywhere, Category = "Classes")
   TSoftClassPtr<ATATCharacter> BlueprintClass;

   // Used to spawn a dummy instance of the character's body in the match lobby screen
   UPROPERTY(EditAnywhere, Category = "Mesh")
   TSoftObjectPtr<USkeletalMesh> CharacterBodyMesh;

   // Used to spawn a dummy instance of the character's head in the match lobby screen
   UPROPERTY(EditAnywhere, Category = "Mesh")
   TSoftObjectPtr<USkeletalMesh> CharacterHeadMesh;

   UPROPERTY(EditAnywhere, Category = "Mesh")
   TSoftObjectPtr<UAnimSequence> CharacterLobbyPose;

   UPROPERTY(EditAnywhere, Category = "Mesh")
   TSoftObjectPtr<UAnimSequence> CharacterAlternateLobbyPose;

   UPROPERTY(EditAnywhere, Category = "Mesh")
   TSoftObjectPtr<UAnimSequence> CharacterWardrobePose;

   UPROPERTY(EditAnywhere, Category = "Art")
   TSoftObjectPtr<UTexture2D> CharacterSelectTexture;

   UPROPERTY(EditAnywhere, Category = "Art")
   TSoftObjectPtr<UTexture2D> CharacterFaceTexture;

#if WITH_EDITOR
   void IsDataValid(FDataValidationContext& context, const FText& errorPrefix) const;
#endif

   inline const FTATCharacterLoadoutConfig* GetLoadoutConfig(ETATLoadoutType loadoutType) const
   {
      if (loadoutType == ETATLoadoutType::Tool) { return &ToolLoadoutConfig; }
      if (loadoutType == ETATLoadoutType::Ability) { return &AbilityLoadoutConfig; }
      if (loadoutType == ETATLoadoutType::Outfit) { return &OutfitLoadoutConfig; }
      return nullptr;
   }
};

USTRUCT(BlueprintType)
struct TAT_API FTATCharacterMetadataHandle
{
   GENERATED_BODY()

   FTATCharacterMetadataHandle() = default;
   FTATCharacterMetadataHandle(TObjectPtr<const UTATCharactersMetadata> asset, ETATCharacter character)
      : _asset(asset), _character(character)
   {}
   
   const FTATCharacterMetadata& Get() const;
   ETATCharacter GetCharacter() const { return _character; }

private:
   UPROPERTY()
   TObjectPtr<const UTATCharactersMetadata> _asset = nullptr;

   UPROPERTY()
   ETATCharacter _character = ETATCharacter::None;
};

// dgraham (1/9/2023) - we should consider that there's many different places in which we enumerate the abilities of a character
// (eg. FOSEAbilityInfo, UTATUpgradeGraph) which could lead to these assets getting out of sync as abilities change. The
// solution isn't immediately clear, but it's something we should address before that happens.
// TODO: revisit this.
UCLASS(BlueprintType)
class TAT_API UTATCharactersMetadata : public UDataAsset
{
   GENERATED_BODY()

public:
#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

   UPROPERTY(EditAnywhere, Category = "Characters")
   TMap<ETATCharacter, FTATCharacterMetadata> Characters;
   
   const FTATCharacterMetadata& GetCharacterMetadata(ETATCharacter character) const;

   UFUNCTION(BlueprintPure, Category = "Characters")
   FTATCharacterMetadataHandle GetCharacterMetadataHandle(ETATCharacter character) const;

   UFUNCTION(BlueprintPure, Category = "Characters")
   bool HasCharacter(ETATCharacter character) const;
   
   UFUNCTION(BlueprintPure="false", Category = "Characters")
   TArray<ETATCharacter> GetAvailableCharacters() const;

   UFUNCTION(BlueprintPure, Category = "Characters")
   bool GetCharacterNameAndDescription(ETATCharacter character, FText& name, FText& className, FText& nickname, FText& description) const;

   UFUNCTION(BlueprintPure, Category = "Characters")
   bool GetCharacterTextures(ETATCharacter character, TSoftObjectPtr<UTexture2D>& symbolTexture, TSoftObjectPtr<UTexture2D>& characterSelectTexture, TSoftObjectPtr<UTexture2D>& characterFaceTexture) const;
};

UCLASS()
class TAT_API UTATCharacterMetadataFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintPure, Category = "Loadouts")
   static UTATCharactersMetadata* GetCharactersMetadataAsset();

   static const FTATCharacterMetadata* FindCharacterMetadataForCharacter(const FTATCharacterSaveId& characterSaveId);

   static const FTATCharacterLoadoutConfig* FindCharacterLoadoutConfigForCharacter(const FTATCharacterSaveId& characterSaveId, ETATLoadoutType loadoutType);

   /// Gets the category of a loadout item. This defines which loadout slots are valid for this item.
   UFUNCTION(BlueprintPure, Category = "Loadouts")
   static FGameplayTag GetLoadoutItemCategory(ETATLoadoutType loadoutType, FGameplayTag loadoutTag);

   UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Loadouts")
   static bool GetAllLoadoutCategories(ETATLoadoutType loadoutType, TArray<FGameplayTag>& loadoutCategories);

   UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Loadouts")
   static bool GetAllLoadoutCategoriesForCharacter(const FTATCharacterDataContext& context, ETATLoadoutType loadoutType, TArray<FTATCharacterLoadoutCategorySpan>& loadoutCategories);

   /// Checks if a loadout is valid for a specific character type
   UFUNCTION(BlueprintPure, Category = "Loadouts")
   static bool IsValidLoadoutForCharacter(const FTATCharacterDataContext& context, ETATLoadoutType loadoutType, const FTATCharacterLoadout& loadout);

   /// Fixes issues with a loadout:
   ///  - Removes invalid entries
   ///  - Ensures the total number of slots is correct
   ///  - When possible, moves entries from a slot they're not allowed in to a slot they are allowed in
   UFUNCTION(BlueprintPure, Category = "Loadouts")
   static void MakeCharacterLoadoutValid(const FTATCharacterDataContext& context, UPARAM(ref) FTATCharacterLoadout& inOutLoadout);

   UFUNCTION(BlueprintPure, Category = "Loadouts")
   static bool GetMaxNumLoadoutSlotsForCharacter(const FTATCharacterDataContext& context, ETATLoadoutType loadoutType, int32& maxSlotCount);

   UFUNCTION(BlueprintPure, Category = "Loadouts")
   static bool GetNumLoadoutSlotsInCategoryForCharacter(const FTATCharacterDataContext& context, ETATLoadoutType loadoutType, UPARAM(Meta = (Categories = "Loadout.Slot")) FGameplayTag loadoutCategory, int32& slotCount);

   /// Gets the loadout category type for a character and slot index
   UFUNCTION(BlueprintPure, Category = "Loadouts")
   static bool GetLoadoutCategoryForSlotIndexAndCharacter(const FTATCharacterDataContext& context, ETATLoadoutType loadoutType, int32 slotIndex, FGameplayTag& loadoutCategory);

   /// Gets all valid loadout entries available to a character for a given slot category
   UFUNCTION(BlueprintPure, Category = "Loadouts")
   static bool GetAvailableLoadoutInCategoryForCharacter(const FTATCharacterDataContext& context, ETATLoadoutType loadoutType, UPARAM(Meta = (Categories = "Loadout.Slot")) FGameplayTag loadoutCategory, TArray<FTATCharacterAvailableLoadoutEntry>& availableLoadout);

   UFUNCTION(BlueprintPure, Category = "Loadouts")
   static bool GetAvailableLoadoutForCharacter(const FTATCharacterDataContext& context, ETATLoadoutType loadoutType, TArray<FTATCharacterAvailableLoadoutEntry>& availableLoadout);

   UFUNCTION(BlueprintPure, Category = "Loadouts")
   static bool GetAvailableLoadoutTagsForCharacter(const FTATCharacterDataContext& context, ETATLoadoutType loadoutType, TArray<FGameplayTag>& availableLoadoutTags, bool filterOutTagsWithUnmetReqs = true);

   UFUNCTION(BlueprintPure, Category = "Loadouts")
   static bool GetDefaultLoadoutForCharacter(const FTATCharacterDataContext& context, ETATLoadoutType loadoutType, FTATCharacterLoadout& defaultLoadout);

   UFUNCTION(BlueprintPure, Category = "Loadout")
   static bool GetLoadoutTagsFeaturedOnCharacterSelectScreen(ETATCharacter character, ETATLoadoutType loadoutType, TArray<FGameplayTag>& characterSelectScreenTags);

   UFUNCTION(BlueprintPure, meta=(WorldContext="worldContext", CallableWithoutWorldContext), Category = "CharacterMetadata|Handle")
   static FTATCharacterMetadataHandle CreateCharacterMetadataHandle(ETATCharacter character, const UObject* worldContext);

   UFUNCTION(BlueprintPure, Category="CharacterMetadata|Handle")
   static ETATCharacter GetCharacter(const FTATCharacterMetadataHandle& handle) { return handle.GetCharacter(); }

   UFUNCTION(BlueprintPure, Category="CharacterMetadata|Handle")
   static FText GetCharacterName(const FTATCharacterMetadataHandle& handle) { return handle.Get().LocalizedName; }
   UFUNCTION(BlueprintPure, Category="CharacterMetadata|Handle")
   static FText GetCharacterClassName(const FTATCharacterMetadataHandle& handle) { return handle.Get().LocalizedClassName; }
   UFUNCTION(BlueprintPure, Category="CharacterMetadata|Handle")
   static FText GetCharacterNickname(const FTATCharacterMetadataHandle& handle) { return handle.Get().LocalizedNickname; }
   UFUNCTION(BlueprintPure, Category="CharacterMetadata|Handle")
   static FText GetCharacterDescription(const FTATCharacterMetadataHandle& handle) { return handle.Get().LocalizedDescription; }
   UFUNCTION(BlueprintPure, Category="CharacterMetadata|Handle")
   static const TSoftObjectPtr<UTexture2D>& GetCharacterSymbolTexture(const FTATCharacterMetadataHandle& handle) { return handle.Get().SymbolTexture; }
   UFUNCTION(BlueprintPure, Category="CharacterMetadata|Handle")
   static const TSoftClassPtr<ATATCharacter>& GetCharacterBlueprintClass(const FTATCharacterMetadataHandle& handle) { return handle.Get().BlueprintClass; }
   UFUNCTION(BlueprintPure, Category="CharacterMetadata|Handle")
   static const TSoftObjectPtr<USkeletalMesh>& GetCharacterBodyMesh(const FTATCharacterMetadataHandle& handle) { return handle.Get().CharacterBodyMesh; }
   UFUNCTION(BlueprintPure, Category="CharacterMetadata|Handle")
   static const TSoftObjectPtr<USkeletalMesh>& GetCharacterHeadMesh(const FTATCharacterMetadataHandle& handle) { return handle.Get().CharacterHeadMesh; }
   UFUNCTION(BlueprintPure, Category="CharacterMetadata|Handle")
   static const TSoftObjectPtr<UAnimSequence>& GetCharacterLobbyPose(const FTATCharacterMetadataHandle& handle) { return handle.Get().CharacterLobbyPose; }
   UFUNCTION(BlueprintPure, Category="CharacterMetadata|Handle")
   static const TSoftObjectPtr<UAnimSequence>& GetCharacterAlternateLobbyPose(const FTATCharacterMetadataHandle& handle) { return handle.Get().CharacterAlternateLobbyPose; }
   UFUNCTION(BlueprintPure, Category="CharacterMetadata|Handle")
   static const TSoftObjectPtr<UAnimSequence>& GetCharacterWardrobePose(const FTATCharacterMetadataHandle& handle) { return handle.Get().CharacterWardrobePose; }
   UFUNCTION(BlueprintPure, Category="CharacterMetadata|Handle")
   static const TSoftObjectPtr<UTexture2D>& GetCharacterSelectTexture(const FTATCharacterMetadataHandle& handle) { return handle.Get().CharacterSelectTexture; }
   UFUNCTION(BlueprintPure, Category="CharacterMetadata|Handle")
   static const TSoftObjectPtr<UTexture2D>& GetCharacterFaceTexture(const FTATCharacterMetadataHandle& handle) { return handle.Get().CharacterFaceTexture; }
};
