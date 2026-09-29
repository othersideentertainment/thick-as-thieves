// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/Inventory/TATLootInventoryListEntryWidget.h"

// ue
#include "Components/ListView.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLootInventoryListEntryWidget)
DEFINE_LOG_CATEGORY_STATIC(LogTATLootInventoryListEntryWidget, Log, All);

void UTATLootInventoryListEntryWidget::NativeOnListItemObjectSet(UObject* listItemObject)
{
   _listViewItem = Cast<UTATSavedLootItemUIProxy>(listItemObject);
   if (_listViewItem != nullptr)
   {
      UpdateContent();
   }
   else
   {
      UE_LOG(LogTATLootInventoryListEntryWidget, Error, TEXT("NativeOnListItemObjectSet failed to cast %s to UTATSavedLootItemUIProxy!"),
         *listItemObject->GetName());
   }

   IUserObjectListEntry::NativeOnListItemObjectSet(listItemObject);
}

FReply UTATLootInventoryListEntryWidget::NativeOnFocusReceived(const FGeometry& inGeometry, const FFocusEvent& inFocusEvent)
{
   const FReply result = Super::NativeOnFocusReceived(inGeometry, inFocusEvent);
   OnFocusStateChanged.ExecuteIfBound(_listViewItem, true);
   return result;
}

void UTATLootInventoryListEntryWidget::NativeOnFocusLost(const FFocusEvent& inFocusEvent)
{
   OnFocusStateChanged.ExecuteIfBound(_listViewItem, false);
   Super::NativeOnFocusLost(inFocusEvent);
}

void UTATLootInventoryListEntryWidget::OnWidgetGeneratedForItem()
{
   check(_listViewItem);
   _listViewItem->OnDataChanged.AddUObject(this, &UTATLootInventoryListEntryWidget::UpdateContent);
   
   _isGenerated = true;
}

void UTATLootInventoryListEntryWidget::OnWidgetReleasedForItem()
{
   if (_listViewItem != nullptr)
   {
      _listViewItem->OnDataChanged.RemoveAll(this);
   }

   ResetToDefaults();

   _isGenerated = false;
}

void UTATLootInventoryListEntryWidget::ResetToDefaults_Implementation()
{
   SetIsSelected(false);
   _listViewItem = nullptr;
   OnFocusStateChanged.Unbind();
   OnInteracted.Unbind();
}

void UTATLootInventoryListEntryWidget::HandleUserInteract()
{
   OnInteracted.Execute(_listViewItem);
}

void UTATLootInventoryListEntryWidget::SetIsSelected(bool isSelected)
{
   if (_isSelected != isSelected)
   {
      _isSelected = isSelected;
      
      UListView* listView = CastChecked<UListView>(GetOwningListView());
      listView->SetItemSelection(_listViewItem, _isSelected);
   }
}
