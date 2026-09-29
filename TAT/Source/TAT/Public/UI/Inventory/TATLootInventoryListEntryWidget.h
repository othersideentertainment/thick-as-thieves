// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "SaveGame/Proxy/TATSavedLootItemUIProxy.h"
#include "UI/TATUserWidget.h"

// ue
#include "Blueprint/IUserObjectListEntry.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "TATLootInventoryListEntryWidget.generated.h"

UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class TAT_API UTATLootInventoryListEntryWidget : public UTATUserWidget,
   public IUserObjectListEntry
{
   GENERATED_BODY()

   DECLARE_DELEGATE_OneParam(FOnListEntryWidgetInteracted, UTATSavedLootItemUIProxy* /*item*/);
   DECLARE_DELEGATE_TwoParams(FOnListEntryWidgetFocusChanged, UTATSavedLootItemUIProxy* /*item*/, bool /*hasFocus*/);

public:
   // from IUserObjectListEntry
   virtual void NativeOnListItemObjectSet(UObject* listItemObject) override;

   void OnWidgetGeneratedForItem();
   void OnWidgetReleasedForItem();

   FORCEINLINE bool HasBeenGenerated() const { return _isGenerated; }

   UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
   void UpdateContent();

   UFUNCTION(BlueprintCallable)
   void HandleUserInteract();

   UFUNCTION(BlueprintCallable)
   FORCEINLINE bool IsSelected() const { return _isSelected; }

   UFUNCTION(BlueprintPure)
   const UTATSavedLootItemUIProxy* GetListObject() const { check(_listViewItem); return _listViewItem; }

   UFUNCTION(BlueprintPure)
   const FTATLootMetadataBP& GetMetadata() const { return GetListObject()->GetMetadata(); }

   UFUNCTION(BlueprintPure)
   const FTATSavedLootItemUIProxyState& GetState() const { return GetListObject()->GetState(); }

   void SetIsSelected(bool isSelected);

   FOnListEntryWidgetFocusChanged OnFocusStateChanged;
   FOnListEntryWidgetInteracted OnInteracted;

protected:
   UFUNCTION(BlueprintNativeEvent)
   void ResetToDefaults();
   void ResetToDefaults_Implementation();

   // from UUserWidget
   virtual FReply NativeOnFocusReceived(const FGeometry& inGeometry, const FFocusEvent& inFocusEvent) override;
   virtual void NativeOnFocusLost(const FFocusEvent& inFocusEvent) override;

private:
   UPROPERTY(Transient)
   UTATSavedLootItemUIProxy* _listViewItem;

   bool _isGenerated = false;
   bool _isSelected = false;

};
