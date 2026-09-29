// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/Inventory/TATLootInventoryListWidget.h"

// tat
#include "UI/Inventory/TATLootInventoryListEntryWidget.h"

// ue
#include "Components/ListView.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLootInventoryListWidget)
DEFINE_LOG_CATEGORY_STATIC(LogTATLootInventoryWidget, Log, All);

struct FListEntrySort
{
   bool operator()(const UObject& entryA, const UObject& entryB) const
   {
      const UTATSavedLootItemUIProxy* itemA = CastChecked<UTATSavedLootItemUIProxy>(&entryA);
      const UTATSavedLootItemUIProxy* itemB = CastChecked<UTATSavedLootItemUIProxy>(&entryB);

      return itemA->GetMetadata().DisplayName.CompareTo(itemB->GetMetadata().DisplayName) < 0;
   }
};

void UTATLootInventoryListWidget::SetEntries(const TArray<UObject*>& entryObjects)
{
   Super::SetEntries(entryObjects);

   _ValidateSelectedEntries();

   _selector.OnListViewEntriesChanged();
}

void UTATLootInventoryListWidget::OnEntriesUpdated(TArray<UObject*>& entries)
{
   entries.Sort(FListEntrySort());
}

void UTATLootInventoryListWidget::NativeOnInitialized()
{
   Super::NativeOnInitialized();

   check(ListView);
   ListView->OnEntryWidgetGenerated().AddUObject(this, &UTATLootInventoryListWidget::_OnWidgetGeneratedForItem);
   ListView->OnEntryWidgetReleased().AddUObject(this, &UTATLootInventoryListWidget::_OnWidgetReleasedForItem);

   _selector.Initialize(this);
}

FReply UTATLootInventoryListWidget::NativeOnFocusReceived(const FGeometry& inGeometry, const FFocusEvent& inFocusEvent)
{
   const FReply result = Super::NativeOnFocusReceived(inGeometry, inFocusEvent);

   // pass focus to the selector entry widget, if necessary
   _selector.TryGrabFocus();

   return result;
}

void UTATLootInventoryListWidget::OnFiltersChanged_Implementation()
{
   Super::OnFiltersChanged_Implementation();

   _ValidateSelectedEntries();

   _selector.OnListViewEntriesChanged();
}

void UTATLootInventoryListWidget::ClearSelection()
{
   const int32 prevSelectedCount = _selectedEntries.Num();
   _selectedEntries.Reset();

   if (prevSelectedCount > 0)
   {
      OnSelectionChanged.Broadcast(_selectedEntries);
   }

   _selector.SetItem(nullptr);
}

UTATLootInventoryListEntryWidget* UTATLootInventoryListWidget::GetWidgetForItem(const UObject* item) const
{
   return CastChecked<UTATLootInventoryListEntryWidget>(GetEntryWidgetForItem(item));
}

UTATLootInventoryListEntryWidget* UTATLootInventoryListWidget::_GetWidgetForItemIfGenerated(const UObject* item) const
{
   if (UTATLootInventoryListEntryWidget* widget = Cast<UTATLootInventoryListEntryWidget>(GetEntryWidgetForItem(item)))
   {
      return widget->HasBeenGenerated() ? widget : nullptr;
   }
   else
   {
      return nullptr;
   }
}

void UTATLootInventoryListWidget::_ValidateSelectedEntries()
{
   const ESelectionMode::Type selectionMode = ListView->GetSelectionMode();
   if (selectionMode != ESelectionMode::Type::Multi)
   {
      bool selectionChanged = false;
      const TArray<UObject*>& listItems = ListView->GetListItems();
      for (auto it = _selectedEntries.CreateIterator(); it; ++it)
      {
         if (!listItems.Contains(*it))
         {
            it.RemoveCurrent();
            selectionChanged = true;
         }
      }

      if (selectionChanged)
      {
         OnSelectionChanged.Broadcast(_selectedEntries);
      }
   }
}

void UTATLootInventoryListWidget::_FilterByLootType(ETATLootType lootType, TArray<UObject*>& itemsToFilter)
{
   for (TArray<UObject*>::TIterator it = itemsToFilter.CreateIterator(); it; ++it)
   {
      const UTATSavedLootItemUIProxy* item = CastChecked<UTATSavedLootItemUIProxy>(*it);
      if (item->GetMetadata().LootType != lootType)
      {
         it.RemoveCurrent();
      }
   }
}

void UTATLootInventoryListWidget::_OnWidgetGeneratedForItem(UUserWidget& widget)
{
   UTATLootInventoryListEntryWidget* listEntryWidget = CastChecked<UTATLootInventoryListEntryWidget>(&widget);
   listEntryWidget->OnFocusStateChanged.BindUObject(this, &UTATLootInventoryListWidget::_OnEntryFocusStateChanged);
   listEntryWidget->OnInteracted.BindUObject(this, &UTATLootInventoryListWidget::_OnEntryInteracted);
   listEntryWidget->OnWidgetGeneratedForItem();

   // If this item was previously selected, resurrect its selection status
   UTATSavedLootItemUIProxy* item = CastChecked<UTATSavedLootItemUIProxy>(UUserObjectListEntryLibrary::GetListItemObject(&widget));
   const bool isAlreadySelected = _selectedEntries.Contains(item);
   listEntryWidget->SetIsSelected(isAlreadySelected);

   if (_selector.GetItem() == item)
   {
      _selector.SetWidget(listEntryWidget);
   }
}

void UTATLootInventoryListWidget::_OnWidgetReleasedForItem(UUserWidget& widget)
{
   UTATLootInventoryListEntryWidget* listEntryWidget = CastChecked<UTATLootInventoryListEntryWidget>(&widget);

   const UTATSavedLootItemUIProxy* listItem = listEntryWidget->GetListObject();
   check(listItem);
   if (_selector.GetItem() == listItem)
   {
      // if the selector item no longer exists, clear it
      const TArray<UObject*>& listItems = ListView->GetListItems();
      if (!listItems.Contains(listItem))
      {
         _selector.SetItem(nullptr);
      }
   }

   listEntryWidget->OnWidgetReleasedForItem();
}

void UTATLootInventoryListWidget::_OnEntryFocusStateChanged(UTATSavedLootItemUIProxy* item, bool hasFocus)
{
   // if a new item has gained focus, cache it
   if (hasFocus)
   {
      _selector.SetItem(item);
   }
}

void UTATLootInventoryListWidget::_OnEntryInteracted(UTATSavedLootItemUIProxy* item)
{
   check(ListView);
   const ESelectionMode::Type selectionMode = ListView->GetSelectionMode();

   const bool isAlreadySelected = _selectedEntries.Contains(item);
   if (isAlreadySelected)
   {
      // if an entry is interacted with while we're in multi-select mode
      // clear the selection if it was previously selected
      if (selectionMode == ESelectionMode::Type::Multi)
      {
         _selectedEntries.Remove(item);

         UTATLootInventoryListEntryWidget* widget = GetWidgetForItem(item);
         widget->SetIsSelected(false);
      }
   }
   else
   {
      // if an entry is interacted with while we're in a single-select mode
      // deselect anything currently selected first
      if (selectionMode != ESelectionMode::Type::Multi)
      {
         for (auto it = _selectedEntries.CreateIterator(); it; ++it)
         {
            // don't use CastChecked (or GetWidgetForItem) as it may be possible that an item was added
            // but was filtered out on its creation, and thus no widget has been generated yet
            if (UTATLootInventoryListEntryWidget* widget = _GetWidgetForItemIfGenerated(*it))
            {
               widget->SetIsSelected(false);
            }

            it.RemoveCurrent();
         }
      }

      _selectedEntries.Add(item);

      UTATLootInventoryListEntryWidget* widget = GetWidgetForItem(item);
      widget->SetIsSelected(true);
   }

   // jump our selector to the just-selected item
   _selector.SetItem(item);

   OnSelectionChanged.Broadcast(_selectedEntries);
}

void UTATLootInventoryListWidget::FSelector::Initialize(UTATLootInventoryListWidget* listWidget)
{
   _listWidget = listWidget;
}

void UTATLootInventoryListWidget::FSelector::OnListViewEntriesChanged()
{
   const UTATLootInventoryListWidget* listWidget = _listWidget.Get();
   check(listWidget != nullptr);

   // if our selector item no longer exists in the list, clear it
   const UTATSavedLootItemUIProxy* selectorItem = _entryItem.Get();
   if (selectorItem != nullptr && !listWidget->GetEntries().Contains(selectorItem))
   {
      // will call TryEnforceValidItem itself
      SetItem(nullptr);
   }
   else
   {
      _TryEnforceValidItem();
   }
}

void UTATLootInventoryListWidget::FSelector::TryGrabFocus()
{
   check(_listWidget.IsValid());

   UTATSavedLootItemUIProxy* selectorItem = _entryItem.Get();
   if (selectorItem == nullptr)
   {
      return;
   }

   // this call serves two functions
   // 1. Scroll to the item if it isn't currently visible
   // 2. Sets SelectorItem in SListView, the object used in navigation code (OnNavigation)
   _listWidget->ListView->RequestNavigateToItem(selectorItem);

   UTATLootInventoryListEntryWidget* selectorWidget = _entryWidget.Get();
   if (selectorWidget == nullptr)
   {
      // may occur if widget has not been generated just yet
      return;
   }

   _EnsureItemIsSelected();

   // only try and pull focus to an entry widget if this owning widget itself/any of its children has focus
   APlayerController* owningPlayer = _listWidget->GetOwningPlayer();
   const bool hasFocus = _listWidget->HasUserFocus(owningPlayer) || _listWidget->HasUserFocusedDescendants(owningPlayer);
   if (hasFocus)
   {
      selectorWidget->SetFocus();
   }
}

void UTATLootInventoryListWidget::FSelector::SetItem(UTATSavedLootItemUIProxy* item)
{
   if (_entryItem.Get() != item)
   {
      _entryItem = item;

      if (item != nullptr)
      {
         check(_listWidget.IsValid());
         UTATLootInventoryListEntryWidget* widget = _listWidget->_GetWidgetForItemIfGenerated(item);
         SetWidget(widget);
      }
      else
      {
         _TryEnforceValidItem();
      }
   }
}

void UTATLootInventoryListWidget::FSelector::SetWidget(UTATLootInventoryListEntryWidget* widget)
{
   check(widget == nullptr || widget->GetListObject() == _entryItem.Get());

   _entryWidget = widget;

   if (_entryWidget.IsValid())
   {
      TryGrabFocus();
   }
}

void UTATLootInventoryListWidget::FSelector::_TryEnforceValidItem()
{
   check(_listWidget.IsValid());
   if (!_entryItem.IsValid())
   {
      // keep it simple, try to use the first item in the list
      const TArray<UObject*>& listItems = _listWidget->ListView->GetListItems();
      if (!listItems.IsEmpty())
      {
         SetItem(CastChecked<UTATSavedLootItemUIProxy>(listItems[0]));
      }
   }

   UE_CLOG(!_entryItem.IsValid(), LogTATLootInventoryWidget, Warning,
      TEXT("FTATLootInventoryListWidgetSelector::TryEnforceValidItem() failed to find a valid item!"));
}

void UTATLootInventoryListWidget::FSelector::_EnsureItemIsSelected()
{
   check(_listWidget.IsValid());
   const ESelectionMode::Type selectionMode = _listWidget->ListView->GetSelectionMode();
   const bool isSingleSelect = (selectionMode == ESelectionMode::Type::Single) 
      || (selectionMode == ESelectionMode::Type::SingleToggle);
   const UTATSavedLootItemUIProxy* selectedItem = _entryItem.Get();
   if (isSingleSelect && selectedItem != nullptr && !_listWidget->_selectedEntries.Contains(selectedItem))
   {
      UTATLootInventoryListEntryWidget* widget = _entryWidget.Get();
      if (widget != nullptr && widget->HasBeenGenerated())
      {
         widget->HandleUserInteract();
      }
   }
}
