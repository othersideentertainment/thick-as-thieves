// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "UI/TATUserWidget.h"

// ue
#include "GameplayTagContainer.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "TATFilteredListViewWrapperWidget.generated.h"

class UListView;

UCLASS(Blueprintable, meta = (DisableNativeTick))
class TAT_API UTATFilteredListViewWrapperWidget : public UTATUserWidget
{
   GENERATED_BODY()

   DECLARE_DYNAMIC_DELEGATE_OneParam(FFilteredListViewWrapperFilterDelegate, TArray<UObject*>&, itemsToFilter);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFiltersChangedEvent, const FGameplayTagContainer&, appliedFilters);

#if WITH_EDITOR
   // from UUserWidget
   virtual void ValidateCompiledWidgetTree(const UWidgetTree& blueprintWidgetTree, class IWidgetCompilerLog& compileLog) const override;
#endif // WITH_EDITOR
	
public:
   UFUNCTION(BlueprintCallable)
   virtual void SetEntries(const TArray<UObject*>& entryObjects);

   UFUNCTION(BlueprintCallable)
   void SetAppliedFilters(const FGameplayTagContainer& filterTags);

   UFUNCTION(BlueprintCallable)
   void ApplyFilter(const FGameplayTag& filterTag);

   UFUNCTION(BlueprintCallable)
   void RevokeFilter(const FGameplayTag& filterTag);

   UFUNCTION(BlueprintCallable)
   void AddFilter(FGameplayTag filterTag, FFilteredListViewWrapperFilterDelegate filterDelegate);

   UFUNCTION(BlueprintCallable)
   void RemoveFilter(FGameplayTag filterTag);

   UFUNCTION(BlueprintCallable)
   void ClearFilters();

   UFUNCTION(BlueprintCallable)
   const FGameplayTagContainer& GetAppliedFilters() const { return _filterTags; }

   UFUNCTION(BlueprintNativeEvent)
   void OnFiltersChanged();
   virtual void OnFiltersChanged_Implementation() { }

   UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
   UListView* ListView;

   UPROPERTY(BlueprintAssignable)
   FOnFiltersChangedEvent OnFiltersChangedEvent;

protected:
   // Called pre-filter refresh, entries will contain all items (regardless of filter)
   virtual void OnEntriesUpdated(TArray<UObject*>& entries) { }

   UUserWidget* GetEntryWidgetForItem(const UObject* item) const;

   FORCEINLINE const TArray<UObject*>& GetEntries() const { return _entries; }

private:
   void _RefreshFilteredEntries(bool filtersChanged);

   UPROPERTY(Transient)
   TArray<UObject*> _entries;

   UPROPERTY(Transient)
   TMap<FGameplayTag, FFilteredListViewWrapperFilterDelegate> _filterDelegates;

   FGameplayTagContainer _filterTags;
};
