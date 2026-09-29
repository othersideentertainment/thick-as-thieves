// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/TATLoadoutWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLoadoutWidget)

DEFINE_LOG_CATEGORY_STATIC(LogTATLoadoutWidget, Log, All);

void UTATLoadoutWidget::NativeConstruct()
{
   Super::NativeConstruct();
}

void UTATLoadoutWidget::NativeDestruct()
{
   Super::NativeDestruct();
}

void UTATLoadoutWidget::SetAvailableLoadout(const TArray<FGameplayTag>& availableLoadoutTags)
{
   _allAvailableLoadoutEntries.Reset();
   _allAvailableLoadoutEntries.Reserve(availableLoadoutTags.Num());

   _availableLoadoutTagToIndex.Reset();
   _availableLoadoutTagToIndex.Reserve(availableLoadoutTags.Num());

   FTATLoadoutWidgetEntryMetadata metadata;
   for (const FGameplayTag& loadoutTag : availableLoadoutTags)
   {
      if (!loadoutTag.IsValid())
      {
         continue;
      }
      else if (_availableLoadoutTagToIndex.Contains(loadoutTag))
      {
         UE_LOG(LogTATLoadoutWidget, Error, TEXT("SetAvailableLoadout: ignoring duplicate loadout tag '%s'"), *loadoutTag.ToString());
         continue;
      }

      metadata.Reset(loadoutTag);
      if (FindMetadataForLoadoutTag(loadoutTag, metadata))
      {
         const int32 entryIndex = _allAvailableLoadoutEntries.Add(metadata);
         _availableLoadoutTagToIndex.Add(loadoutTag, entryIndex);
      }
   }

   OnAvailableLoadoutChanged();
}

void UTATLoadoutWidget::SetLoadoutSlotCategoryRequirements(const TArray<FTATCharacterLoadoutCategorySpan>& categorySpans)
{
   _slotCategoryRequirements = categorySpans;
}

bool UTATLoadoutWidget::GetCategoryRequirementForSlot(int32 loadoutIndex, FGameplayTag& slotCategory) const
{
   for (const FTATCharacterLoadoutCategorySpan& span : _slotCategoryRequirements)
   {
      if (span.ContainsIndex(loadoutIndex))
      {
         slotCategory = span.Category;
         return slotCategory.IsValid();
      }
   }
   slotCategory = FGameplayTag::EmptyTag;
   return false;
}

void UTATLoadoutWidget::SetCurrentLoadout(const FTATCharacterLoadout& newLoadout, int32 minimumSlotCount)
{
   _currentLoadout = newLoadout;

   // Just a sanity bounds check because we might be about to set an array to this size
   constexpr int32 maxSlotCount = 8192;
   ensure(minimumSlotCount <= maxSlotCount);
   minimumSlotCount = FMath::Clamp(minimumSlotCount, 0, maxSlotCount);

   if (_currentLoadout.Entries.Num() < minimumSlotCount)
   {
      _currentLoadout.Entries.SetNum(minimumSlotCount);
   }

   OnEntireLoadoutChanged();
}

bool UTATLoadoutWidget::FindMetadataForLoadoutTag_Implementation(FGameplayTag loadoutTag, FTATLoadoutWidgetEntryMetadata& entryMetadata) const
{
   entryMetadata.Reset(loadoutTag);
   return true;
}

bool UTATLoadoutWidget::IsValidLoadoutAssignment_Implementation(int32 loadoutIndex, FGameplayTag loadoutTag, FText& errorMessage) const
{
   errorMessage = FText::GetEmpty();
   return true;
}

bool UTATLoadoutWidget::GetAvailableLoadoutEntryByIndex(int32 index, FTATLoadoutWidgetEntryMetadata& loadoutEntryMetadata) const
{
   if (_allAvailableLoadoutEntries.IsValidIndex(index))
   {
      loadoutEntryMetadata = _allAvailableLoadoutEntries[index];
      return true;
   }
   loadoutEntryMetadata = FTATLoadoutWidgetEntryMetadata{};
   return false;
}

bool UTATLoadoutWidget::GetAvailableLoadoutEntryByLoadoutTag(FGameplayTag loadoutTag, int32& availableEntryIndex, FTATLoadoutWidgetEntryMetadata& entryMetadata) const
{
   if (const int32* index = _availableLoadoutTagToIndex.Find(loadoutTag))
   {
      check(_allAvailableLoadoutEntries.IsValidIndex(*index));
      availableEntryIndex = *index;
      entryMetadata = _allAvailableLoadoutEntries[*index];
      return true;
   }
   availableEntryIndex = 0;
   entryMetadata = FTATLoadoutWidgetEntryMetadata{};
   return false;
}

bool UTATLoadoutWidget::GetLoadoutEntry(int32 index, FTATCharacterLoadoutEntry& entry) const
{
   if (_currentLoadout.Entries.IsValidIndex(index))
   {
      entry = _currentLoadout.Entries[index];
      return true;
   }
   entry = FTATCharacterLoadoutEntry{};
   return false;
}

bool UTATLoadoutWidget::GetLoadoutEntryMetadata(int32 loadoutIndex, FTATLoadoutWidgetEntryMetadata& metadata) const
{
   if (_currentLoadout.Entries.IsValidIndex(loadoutIndex))
   {
      if (const int32* index = _availableLoadoutTagToIndex.Find(_currentLoadout.Entries[loadoutIndex].LoadoutTag))
      {
         check(_allAvailableLoadoutEntries.IsValidIndex(*index));
         metadata = _allAvailableLoadoutEntries[*index];
         return true;
      }
   }
   metadata = FTATLoadoutWidgetEntryMetadata{};
   return false;
}

bool UTATLoadoutWidget::FindFirstLoadoutIndexWithTag(FGameplayTag loadoutTag, int32& loadoutIndex) const
{
   for (int32 i = 0; i < _currentLoadout.Entries.Num(); i++)
   {
      if (_currentLoadout.Entries[i].LoadoutTag == loadoutTag)
      {
         loadoutIndex = i;
         return true;
      }
   }
   loadoutIndex = INDEX_NONE;
   return false;
}

int32 UTATLoadoutWidget::NumLoadoutEntriesWithTag(FGameplayTag loadoutTag) const
{
   int32 result = 0;
   for (int32 i = 0; i < _currentLoadout.Entries.Num(); i++)
   {
      if (_currentLoadout.Entries[i].LoadoutTag == loadoutTag)
      {
         ++result;
      }
   }
   return result;
}

bool UTATLoadoutWidget::ClearLoadoutEntry(int32 loadoutIndex)
{
   if (_currentLoadout.Entries.IsValidIndex(loadoutIndex))
   {
      _currentLoadout.Entries[loadoutIndex] = FTATCharacterLoadoutEntry{};
      OnLoadoutEntryChanged(loadoutIndex);
      return true;
   }
   return false;
}

bool UTATLoadoutWidget::SetLoadoutEntryInputAction(int32 loadoutIndex, ETATCharacterInputActionType inputActionType)
{
   if (_currentLoadout.Entries.IsValidIndex(loadoutIndex))
   {
      _currentLoadout.Entries[loadoutIndex].InputAction = inputActionType;
      OnLoadoutEntryChanged(loadoutIndex);
      return true;
   }
   return false;
}

bool UTATLoadoutWidget::AssignLoadoutEntryByAvailableEntryIndex(int32 loadoutIndex, int32 availableEntryIndex)
{
   if (!_currentLoadout.Entries.IsValidIndex(loadoutIndex) || !_allAvailableLoadoutEntries.IsValidIndex(availableEntryIndex))
   {
      return false;
   }
   return _AssignLoadoutEntry(loadoutIndex, _allAvailableLoadoutEntries[availableEntryIndex].LoadoutTag);
}

bool UTATLoadoutWidget::AssignLoadoutEntryByLoadoutTag(int32 loadoutIndex, FGameplayTag loadoutTag, bool requireTagInAvailableLoadout)
{
   if (!_currentLoadout.Entries.IsValidIndex(loadoutIndex) || (requireTagInAvailableLoadout && !_availableLoadoutTagToIndex.Contains(loadoutTag)))
   {
      return false;
   }
   return _AssignLoadoutEntry(loadoutIndex, loadoutTag);
}

bool UTATLoadoutWidget::MoveLoadoutEntryUp(int32 loadoutIndex)
{
   if (_currentLoadout.Entries.Num() < 2 || !_currentLoadout.Entries.IsValidIndex(loadoutIndex) || loadoutIndex <= 0)
   {
      return false;
   }
   const int32 targetIndex = loadoutIndex - 1;
   return _SwapLoadoutEntry(loadoutIndex, targetIndex);
}

bool UTATLoadoutWidget::MoveLoadoutEntryDown(int32 loadoutIndex)
{
   if (_currentLoadout.Entries.Num() < 2 || !_currentLoadout.Entries.IsValidIndex(loadoutIndex) || loadoutIndex >= _currentLoadout.Entries.Num() - 1)
   {
      return false;
   }
   const int32 targetIndex = loadoutIndex + 1;
   return _SwapLoadoutEntry(loadoutIndex, targetIndex);
}

int UTATLoadoutWidget::GetTotalSlotsForCategory(FGameplayTag slotCategory) const
{
   if (!slotCategory.IsValid())
   {
      UE_LOG(LogTATLoadoutWidget, Error, TEXT("GetTotalSlotsForCategory() called with invalid tag!"));
      return INDEX_NONE;
   }

   const FTATCharacterLoadoutCategorySpan* slotCategorySpan = _GetLoadoutSpanForCategory(slotCategory);
   if (!slotCategorySpan)
   {
      UE_LOG(LogTATLoadoutWidget, Error, TEXT("GetTotalSlotsForCategory() failed to find slot category for tag %s!"), *slotCategory.ToString());
      return INDEX_NONE;
   }
   return slotCategorySpan->SlotCount;
}

int UTATLoadoutWidget::GetOccupiedSlotsForCategory(FGameplayTag slotCategory) const
{
   if (!slotCategory.IsValid())
   {
      UE_LOG(LogTATLoadoutWidget, Error, TEXT("GetOccupiedSlotsForCategory() called with invalid tag!"));
      return INDEX_NONE;
   }

   const FTATCharacterLoadoutCategorySpan* slotCategorySpan = _GetLoadoutSpanForCategory(slotCategory);
   if (!slotCategorySpan)
   {
      UE_LOG(LogTATLoadoutWidget, Error, TEXT("GetOccupiedSlotsForCategory() could not find slot category for tag %s!"), *slotCategory.ToString());
      return INDEX_NONE;
   }
   
   const int32 startIndex = slotCategorySpan->StartIndex;
   const int32 slotCount = slotCategorySpan->SlotCount;
   int32 occupiedSlots = 0;
   FTATCharacterLoadoutEntry loadoutEntry;
   for (int32 slotIndex = startIndex; slotIndex < startIndex + slotCount; slotIndex++)
   {
      if (GetLoadoutEntry(slotIndex, loadoutEntry))
      {
         if (loadoutEntry.LoadoutTag.IsValid())
         {
            occupiedSlots++;
         }
      }
   }
   return occupiedSlots;
}

bool UTATLoadoutWidget::SwapLoadoutEntries(int32 loadoutIndexA, int32 loadoutIndexB)
{
   if (_currentLoadout.Entries.Num() < 2 || loadoutIndexA == loadoutIndexB || !_currentLoadout.Entries.IsValidIndex(loadoutIndexA) || !_currentLoadout.Entries.IsValidIndex(loadoutIndexB))
   {
      return false;
   }
   if (_currentLoadout.Entries[loadoutIndexA] != _currentLoadout.Entries[loadoutIndexB])
   {
      return _SwapLoadoutEntry(loadoutIndexA, loadoutIndexB);
   }
   return false;
}

bool UTATLoadoutWidget::_AssignLoadoutEntry(int32 loadoutIndex, FGameplayTag loadoutTag)
{
   check(_currentLoadout.Entries.IsValidIndex(loadoutIndex));

   FText errorMessage;
   if (!IsValidLoadoutAssignment(loadoutIndex, loadoutTag, errorMessage))
   {
      if (TryHandleInvalidLoadoutAssignment(loadoutIndex, loadoutTag))
      {
         return true;
      }

      if (!errorMessage.IsEmpty())
      {
         OnLoadoutAssignmentError(loadoutTag, errorMessage);
      }
      return false;
   }

   _currentLoadout.Entries[loadoutIndex].LoadoutTag = loadoutTag;
   OnLoadoutEntryChanged(loadoutIndex);
   return true;
}

bool UTATLoadoutWidget::_SwapLoadoutEntry(int32 sourceIndex, int32 targetIndex)
{
   checkf(_currentLoadout.Entries.IsValidIndex(sourceIndex) && _currentLoadout.Entries.IsValidIndex(targetIndex) && sourceIndex != targetIndex,
      TEXT("UTATLoadoutWidget::_SwapLoadoutEntry(src=%i, tgt=%i) is not a valid operation for a loadout of size %i"),
      sourceIndex, targetIndex, _currentLoadout.Entries.Num());

   // cache both current entries so we can safely clear/restore data
   const FTATCharacterLoadoutEntry origSourceEntry = _currentLoadout.Entries[sourceIndex];
   const FTATCharacterLoadoutEntry origTargetEntry = _currentLoadout.Entries[targetIndex];

   // Nothing to swap if both tags are invalid
   if (!origSourceEntry.LoadoutTag.IsValid() && !origTargetEntry.LoadoutTag.IsValid())
   {
      return true;
   }

   FText errorMessage;

   // clear both slots temporarily so the blueprint validation function sees them as empty
   _currentLoadout.Entries[targetIndex] = {};
   _currentLoadout.Entries[sourceIndex] = {};
   auto restoreOrigEntries = [&]()
   {
      _currentLoadout.Entries[targetIndex] = origTargetEntry;
      _currentLoadout.Entries[sourceIndex] = origSourceEntry;
   };

   // check if we can assign source -> target
   if (!IsValidLoadoutAssignment(targetIndex, origSourceEntry.LoadoutTag, errorMessage))
   {
      if (!errorMessage.IsEmpty())
      {
         OnLoadoutAssignmentError(origSourceEntry.LoadoutTag, errorMessage);
      }
      restoreOrigEntries();
      return false;
   }

   // clear the source entry slot so the validation function sees it as empty

   // Check if we can assign target -> source
   errorMessage = FText::GetEmpty();
   if (!IsValidLoadoutAssignment(sourceIndex, origTargetEntry.LoadoutTag, errorMessage))
   {
      if (!errorMessage.IsEmpty())
      {
         OnLoadoutAssignmentError(origTargetEntry.LoadoutTag, errorMessage);
      }
      restoreOrigEntries();
      return false;
   }

   // at this point we know that both assignments will succeed
   _currentLoadout.Entries[sourceIndex] = origTargetEntry;
   _currentLoadout.Entries[targetIndex] = origSourceEntry;
   OnLoadoutEntryChanged(sourceIndex);
   OnLoadoutEntryChanged(targetIndex);
   return true;
}

const FTATCharacterLoadoutCategorySpan* UTATLoadoutWidget::_GetLoadoutSpanForCategory(FGameplayTag slotCategory) const
{
   if (!slotCategory.IsValid())
   {
      UE_LOG(LogTATLoadoutWidget, Error, TEXT("_GetLoadoutSpanForCategory() called with invalid slotCategory tag!"));
      return nullptr;
   }

   for (const FTATCharacterLoadoutCategorySpan& categorySpan : _slotCategoryRequirements)
   {
      if (categorySpan.Category == slotCategory)
      {
         return &categorySpan;
      }
   }

   UE_LOG(LogTATLoadoutWidget, Error, TEXT("_GetLoadoutSpanForCategory() called with category %s not present in _slotCategoryRequirements!"), *slotCategory.ToString());
   return nullptr;
}
