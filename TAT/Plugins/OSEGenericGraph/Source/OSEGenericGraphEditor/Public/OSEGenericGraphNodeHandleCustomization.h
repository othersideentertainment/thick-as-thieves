// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class UOSEGenericGraph;
class UOSEGenericGraphNode;
class UOSEGenericGraphEdGraph;

struct FOSEGenericGraphNodeHandleComboBoxEntry
{
   FText Label;
   FGuid NodeId;
   TOptional<FIntVector2> NodePos;
};

class FOSEGenericGraphNodeHandleCustomization : public IPropertyTypeCustomization
{
public:
   FOSEGenericGraphNodeHandleCustomization();

   static TSharedRef<IPropertyTypeCustomization> MakeInstance();

   // From IPropertyTypeCustomization
   virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
   virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

protected:
   TSharedPtr<IPropertyHandle> StructPropHandle;
   TSharedPtr<IPropertyHandle> GraphPropHandle;
   TSharedPtr<IPropertyHandle> NodePropHandle;
   bool IsSoftHandle = false;

   TSharedPtr<SComboBox<TSharedPtr<FOSEGenericGraphNodeHandleComboBoxEntry>>> NodeComboBox;
   TArray<TSharedPtr<FOSEGenericGraphNodeHandleComboBoxEntry>> NodeComboList;

   void OnGraphChanged();
   bool ShouldFilterAsset(const FAssetData& AssetData);

   /// Try getting the current graph if it's already loaded, otherwise return null
   UOSEGenericGraph* TryGetCurrentGraphIfLoaded() const;

   /// Get the current graph. If it's a soft object ptr, load it synchronously and return it
   UOSEGenericGraph* LoadCurrentGraphSynchronous() const;

   /// Gets the current node guid
   TOptional<FGuid> GetCurrentNodeId() const;

   void SetCurrentNodeId(const FGuid& NodeId);

   void OnNodeChanged(TSharedPtr<FOSEGenericGraphNodeHandleComboBoxEntry> NewSelection, ESelectInfo::Type SelectInfo);
   TSharedRef<SWidget> MakeNodeEntryWidget(TSharedPtr<FOSEGenericGraphNodeHandleComboBoxEntry> InItem);

   void GetNodeOptions(UOSEGenericGraph* Graph, TArray<TSharedPtr<FOSEGenericGraphNodeHandleComboBoxEntry>>& OutEntries) const;
   void RebuildNodeOptionList(UOSEGenericGraph* Graph);
   void OnOpenNodeSelectComboBox();
   FReply OnRefreshButtonClicked();
   FReply OnSoftHandleLoadButtonClicked();
   bool IsSoftHandleLoadButtonEnabled() const;
   FText GetHeaderRowText() const;
   FText GetCurrentNodeLabel() const;

   static TOptional<FIntVector2> GetNodePosition(UOSEGenericGraphEdGraph* EdGraph, UOSEGenericGraphNode* Node);

   // The MetaData derived filter for the row type
   FName GraphTypeFilter;
   UClass* GraphFilterClass = nullptr;
   UScriptStruct* GraphFilterScriptStruct = nullptr;

};
