// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/TATFilteredListViewWrapperWidget.h"

// ue
#include "Components/ListView.h"
#include "Editor/WidgetCompilerLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATFilteredListViewWrapperWidget)
DEFINE_LOG_CATEGORY_STATIC(LogTATFilteredListViewWrapperWidget, Log, All);

#if WITH_EDITOR
void UTATFilteredListViewWrapperWidget::ValidateCompiledWidgetTree(const UWidgetTree& blueprintWidgetTree, IWidgetCompilerLog& compileLog) const
{
   Super::ValidateCompiledWidgetTree(blueprintWidgetTree, compileLog);

   static const FName widgetName = GET_MEMBER_NAME_CHECKED(UTATFilteredListViewWrapperWidget, ListView);
   if (!blueprintWidgetTree.FindWidget(widgetName))
   {
      compileLog.Error(FText::FromString(FString::Printf(TEXT("[%s] requires there to exist a UListView-inheriting child widget with the name '%s'!")
         , *GetName(), *widgetName.ToString())));
   }
}
#endif // WITH_EDITOR

void UTATFilteredListViewWrapperWidget::SetEntries(const TArray<UObject*>& entryObjects)
{
   _entries = entryObjects;
   OnEntriesUpdated(_entries);

   const bool filtersChanged = false;
   _RefreshFilteredEntries(filtersChanged);
}

void UTATFilteredListViewWrapperWidget::SetAppliedFilters(const FGameplayTagContainer& filterTags)
{
   if (_filterTags != filterTags)
   {
      _filterTags = filterTags;

      const bool filtersChanged = true;
      _RefreshFilteredEntries(filtersChanged);
   }
}

void UTATFilteredListViewWrapperWidget::ApplyFilter(const FGameplayTag& filterTag)
{
   const int32 count = _filterTags.Num();
   _filterTags.AddTag(filterTag);
   if (count != _filterTags.Num())
   {
      const bool filtersChanged = true;
      _RefreshFilteredEntries(filtersChanged);
   }
}

void UTATFilteredListViewWrapperWidget::RevokeFilter(const FGameplayTag& filterTag)
{
   if (_filterTags.RemoveTag(filterTag))
   {
      const bool filtersChanged = true;
      _RefreshFilteredEntries(filtersChanged);
   }
}

void UTATFilteredListViewWrapperWidget::AddFilter(FGameplayTag filterTag, FFilteredListViewWrapperFilterDelegate filterDelegate)
{
   _filterDelegates.Emplace(filterTag, filterDelegate);
}

void UTATFilteredListViewWrapperWidget::RemoveFilter(FGameplayTag filterTag)
{
   if (_filterDelegates.Remove(filterTag) > 0)
   {
      // ensure the filter being removed isn't still applied
      RevokeFilter(filterTag);
   }
}

void UTATFilteredListViewWrapperWidget::ClearFilters()
{
   _filterDelegates.Empty();

   // remove any filters previously applied
   if (!_filterTags.IsEmpty())
   {
      _filterTags.Reset();
      _RefreshFilteredEntries(true);
   }
}

UUserWidget* UTATFilteredListViewWrapperWidget::GetEntryWidgetForItem(const UObject* item) const
{
   check(ListView);
   return ListView->GetEntryWidgetFromItem(item);
}

void UTATFilteredListViewWrapperWidget::_RefreshFilteredEntries(bool filtersChanged)
{
   check(ListView);

   const bool wasEmpty = ListView->GetNumItems() == 0;
   if (_filterTags.IsEmpty())
   {
      // If there are no filters, we can display everything
      ListView->SetListItems(_entries);
   }
   else
   {
      TArray<UObject*> visibleEntries = _entries;
      for (const FGameplayTag& filterTag : _filterTags)
      {
         if (const auto& it = _filterDelegates.Find(filterTag))
         {
            it->Execute(visibleEntries);
         }
      }
      ListView->SetListItems(visibleEntries);
   }

   const bool isEmpty = ListView->GetNumItems() == 0;
   if (!wasEmpty && isEmpty)
   {
      // when UListView is passed an empty array via SetListItems, it (surprisingly) will not release any previous entry widgets
      // WidgetGenerator.OnBeginGenerationPass() inside STableViewBase ReGenerateItems is what marks widgets for release, but
      // it does not run if the list does not contain any items
      // to rectify, if the list is newly empty, force it to release the widgets into the inative pool
      ListView->RegenerateAllEntries();
   }

   if (filtersChanged)
   {
      OnFiltersChanged();
      OnFiltersChangedEvent.Broadcast(_filterTags);
   }
}
