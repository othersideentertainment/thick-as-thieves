// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "CharacterCustomization/TATCharacterLoadout.h"
#include "UI/TATUserWidget.h"
#include "Character/TATCharacterMetadata.h"

#include "TATLoadoutWidget.generated.h"

class UPaperSprite;

/// Metadata for the UI to display something that's available for a player to use in their loadout
USTRUCT(BlueprintType)
struct TAT_API FTATLoadoutWidgetEntryMetadata
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loadout Entry")
   FGameplayTag LoadoutTag;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loadout Entry")
   FText DisplayName;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loadout Entry")
   FText Description;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loadout Entry", meta = (DisplayThumbnail = "true"))
   TSoftObjectPtr<UPaperSprite> IconSprite;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loadout Entry", meta = (Categories = "Loadout.Slot"))
   FGameplayTag SlotCategory;

   void Reset(FGameplayTag newLoadoutTag = FGameplayTag::EmptyTag)
   {
      LoadoutTag = newLoadoutTag;
      DisplayName = newLoadoutTag.IsValid() ? FText::AsCultureInvariant(newLoadoutTag.ToString()) : FText::GetEmpty();
      Description = FText::GetEmpty();
      IconSprite.Reset();
      SlotCategory = FGameplayTag::EmptyTag;
   }
};

/// Widget for displaying and editing loadouts
UCLASS(meta = (DisableNativeTick))
class TAT_API UTATLoadoutWidget : public UTATUserWidget
{
   GENERATED_BODY()
   
public:
   // from UUserWidget
   virtual void NativeConstruct() override;
   virtual void NativeDestruct() override;

   UFUNCTION(BlueprintCallable, Category = "Loadout Widget")
   void SetAvailableLoadout(const TArray<FGameplayTag>& availableLoadoutTags);

   /// Sets category-based slot requirements
   UFUNCTION(BlueprintCallable, Category = "Loadout Widget")
   void SetLoadoutSlotCategoryRequirements(const TArray<FTATCharacterLoadoutCategorySpan>& categorySpans);

   /// Gets the required category for a loadout slot, if any
   UFUNCTION(BlueprintPure, Category = "Loadout Widget")
   bool GetCategoryRequirementForSlot(int32 loadoutIndex, FGameplayTag& slotCategory) const;

   /// Sets the current loadout.
   /// If the number of slots in the new loadout is less than the minimum slot count, empty tags are added to the end to fill it out.
   UFUNCTION(BlueprintCallable, Category = "Loadout Widget")
   void SetCurrentLoadout(const FTATCharacterLoadout& newLoadout, int32 minimumSlotCount = 1);

   /// Gets a copy of the current loadout that can be passed to the save game
   UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Loadout Widget")
   FORCEINLINE FTATCharacterLoadout GetCurrentLoadout() const { return _currentLoadout; }

   /// Subclasses should override this to define what loadout type this widget is managing.
   UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Loadout Widget")
   ETATLoadoutType GetLoadoutType() const;
   virtual ETATLoadoutType GetLoadoutType_Implementation() const { return ETATLoadoutType::Generic; }

protected:
   /// Subclasses should override this to provide their own lookup to get display metadata for loadout tags.
   /// Note that if this returns false, it will effectively "veto" an available loadout item from being available in the UI. Only return false for invalid items.
   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintPure = false, Category = "Loadout Widget")
   bool FindMetadataForLoadoutTag(FGameplayTag loadoutTag, FTATLoadoutWidgetEntryMetadata& entryMetadata) const;
   virtual bool FindMetadataForLoadoutTag_Implementation(FGameplayTag loadoutTag, FTATLoadoutWidgetEntryMetadata& entryMetadata) const;

   /// Subclasses should override this to perform validation to make sure a loadout item can be assigned.
   /// Eg. this could check if the loadout already has an entry with the same tag
   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Loadout Widget")
   bool IsValidLoadoutAssignment(int32 loadoutIndex, FGameplayTag loadoutTag, FText& errorMessage) const;
   virtual bool IsValidLoadoutAssignment_Implementation(int32 loadoutIndex, FGameplayTag loadoutTag, FText& errorMessage) const;

   /// Event fired when a loadout assignment failed and we have an error message we can show the player (eg. "You already have that item assigned")
   UFUNCTION(BlueprintNativeEvent, Category = "Loadout Widget")
   void OnLoadoutAssignmentError(FGameplayTag loadoutTag, const FText& errorMessage);
   virtual void OnLoadoutAssignmentError_Implementation(FGameplayTag loadoutTag, const FText& errorMessage) {}

   /// Event fired when a loadout assignment failed to try and recover from it.
   /// For example, if a tag would be a duplicate assignment, you could swap this index with the other one instead of failing.
   /// Return true only if a successful recovery was made.
   UFUNCTION(BlueprintNativeEvent, Category = "Loadout Widget")
   bool TryHandleInvalidLoadoutAssignment(int32 loadoutIndex, FGameplayTag loadoutTag);
   virtual bool TryHandleInvalidLoadoutAssignment_Implementation(int32 loadoutIndex, FGameplayTag loadoutTag) { return false; }

   /// When the available loadout has changed (eg. SetAvailableLoadout was called)
   UFUNCTION(BlueprintNativeEvent, Category = "Loadout Widget")
   void OnAvailableLoadoutChanged();
   void OnAvailableLoadoutChanged_Implementation() {}

   /// When the entire loadout has changed (eg. SetEntireLoadout was called)
   UFUNCTION(BlueprintNativeEvent, Category = "Loadout Widget")
   void OnEntireLoadoutChanged();
   void OnEntireLoadoutChanged_Implementation() {}

   /// When one entry in the selected loadout has changed
   UFUNCTION(BlueprintNativeEvent, Category = "Loadout Widget")
   void OnLoadoutEntryChanged(int32 loadoutIndex);
   void OnLoadoutEntryChanged_Implementation(int32 loadoutIndex) {}

   UFUNCTION(BlueprintPure, Category = "Loadout Widget")
   FORCEINLINE int32 GetNumAvailableLoadoutEntries() const { return _allAvailableLoadoutEntries.Num(); }

   UFUNCTION(BlueprintPure, Category = "Loadout Widget")
   bool GetAvailableLoadoutEntryByIndex(int32 index, FTATLoadoutWidgetEntryMetadata& loadoutEntryMetadata) const;

   UFUNCTION(BlueprintPure, Category = "Loadout Widget")
   bool GetAvailableLoadoutEntryByLoadoutTag(FGameplayTag loadoutTag, int32& availableEntryIndex, FTATLoadoutWidgetEntryMetadata& entryMetadata) const;

   UFUNCTION(BlueprintPure, Category = "Loadout Widget")
   int32 GetLoadoutSize() const { return _currentLoadout.Entries.Num(); }

   UFUNCTION(BlueprintPure, Category = "Loadout Widget")
   bool GetLoadoutEntry(int32 loadoutIndex, FTATCharacterLoadoutEntry& entry) const;

   UFUNCTION(BlueprintPure, Category = "Loadout Widget")
   bool GetLoadoutEntryMetadata(int32 loadoutIndex, FTATLoadoutWidgetEntryMetadata& metadata) const;

   UFUNCTION(BlueprintPure, Category = "Loadout Widget")
   bool FindFirstLoadoutIndexWithTag(FGameplayTag loadoutTag, int32& loadoutIndex) const;

   UFUNCTION(BlueprintPure, Category = "Loadout Widget")
   int32 NumLoadoutEntriesWithTag(FGameplayTag loadoutTag) const;

   /// Sets a loadout entry to an empty loadout tag
   UFUNCTION(BlueprintCallable, Category = "Loadout Widget")
   bool ClearLoadoutEntry(int32 loadoutIndex);

   UFUNCTION(BlueprintCallable, Category = "Loadout Widget")
   bool SetLoadoutEntryInputAction(int32 loadoutIndex, ETATCharacterInputActionType inputActionType);

   UFUNCTION(BlueprintCallable, Category = "Loadout Widget")
   bool AssignLoadoutEntryByAvailableEntryIndex(int32 loadoutIndex, int32 availableEntryIndex);

   UFUNCTION(BlueprintCallable, Category = "Loadout Widget")
   bool AssignLoadoutEntryByLoadoutTag(int32 loadoutIndex, FGameplayTag loadoutTag, bool requireTagInAvailableLoadout = true);

   UFUNCTION(BlueprintCallable, Category = "Loadout Widget")
   bool MoveLoadoutEntryUp(int32 loadoutIndex);

   UFUNCTION(BlueprintCallable, Category = "Loadout Widget")
   bool MoveLoadoutEntryDown(int32 loadoutIndex);

   /// Returns total number slots for given category, or INDEX_NONE if category is invalid
   UFUNCTION(BlueprintCallable, Category = "Loadout Widget")
   int GetTotalSlotsForCategory(UPARAM(Meta = (Categories = "Loadout.Slot")) FGameplayTag slotCategory) const;

   /// Returns number of occupied slots for given category, or INDEX_NONE if category is invalid
   UFUNCTION(BlueprintCallable, Category = "Loadout Widget")
   int GetOccupiedSlotsForCategory(UPARAM(Meta = (Categories = "Loadout.Slot")) FGameplayTag slotCategory) const;

   /// Swaps two loadout entries.
   /// If swapTagOnly is true, only the tag is swapped, otherwise all values in the entry are swapped.
   UFUNCTION(BlueprintCallable, Category = "Loadout Widget")
   bool SwapLoadoutEntries(int32 loadoutIndexA, int32 loadoutIndexB);

private:
   bool _AssignLoadoutEntry(int32 loadoutIndex, FGameplayTag loadoutTag);

   bool _SwapLoadoutEntry(int32 sourceIndex, int32 targetIndex);

   const FTATCharacterLoadoutCategorySpan* _GetLoadoutSpanForCategory(FGameplayTag slotCategory) const;

   UPROPERTY(Transient)
   FTATCharacterLoadout _currentLoadout;

   UPROPERTY(Transient)
   TArray<FTATCharacterLoadoutCategorySpan> _slotCategoryRequirements;

   UPROPERTY(Transient)
   TArray<FTATLoadoutWidgetEntryMetadata> _allAvailableLoadoutEntries;

   /// Map of loadout tag to its index in the _allAvailableLoadoutEntries array
   TMap<FGameplayTag, int32> _availableLoadoutTagToIndex;
};
