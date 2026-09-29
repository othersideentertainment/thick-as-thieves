// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2018-2024 OtherSide Entertainment, Inc. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#include "OSEGenericGraphNodeHandleCustomization.h"
#include "PropertyHandle.h"
#include "IDetailChildrenBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailLayoutBuilder.h"
#include "EditorFontGlyphs.h"
#include "KismetCastingUtils.h"
#include "OSEGenericGraph.h"
#include "OSEGenericGraphNodeHandle.h"
#include "OSEGenericGraphSoftNodeHandle.h"
#include "PropertyCustomizationHelpers.h"
#include "ScopedTransaction.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEdGraph.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEdNode.h"
#include "Serialization/MappedName.h"

#define LOCTEXT_NAMESPACE "OSEGenericGraphNodeHandleCustomization"

namespace GenericGraphHelpers
{
   template<typename T>
   bool PropToObjectPtr(TSharedPtr<IPropertyHandle> Prop, T*& OutValue)
   {
      UObject* Result = nullptr;
      if (!Prop || !Prop->IsValidHandle() || Prop->GetValue(Result) != FPropertyAccess::Success)
      {
         OutValue = nullptr;
         return false;
      }
      OutValue = Cast<T>(Result);
      return true;
   }

   template<typename T>
   bool PropToSoftObjectPtr(TSharedPtr<IPropertyHandle> Prop, TSoftObjectPtr<T>& OutValue)
   {
      OutValue.Reset();
      if (!Prop || !Prop->IsValidHandle())
      {
         return false;
      }
      const FSoftObjectProperty* SoftObjectProperty = CastField<const FSoftObjectProperty>(Prop->GetProperty());
      if (SoftObjectProperty == nullptr)
      {
         return false;
      }
      void* DataPtr = nullptr;
      if (Prop->GetValueData(DataPtr) != FPropertyAccess::Success || DataPtr == nullptr)
      {
         return false;
      }
      const FSoftObjectPtr& Value = SoftObjectProperty->GetPropertyValue_InContainer(DataPtr);
      OutValue = TSoftObjectPtr<T>{ Value.ToSoftObjectPath() };
      return true;
   }

   bool PropToGuid(TSharedPtr<IPropertyHandle> Prop, FGuid& OutValue)
   {
      void* DataPtr = nullptr;
      if (!Prop || !Prop->IsValidHandle() || Prop->GetValueData(DataPtr) != FPropertyAccess::Success || DataPtr == nullptr)
      {
         OutValue = FGuid();
         return false;
      }
      OutValue = *static_cast<FGuid*>(DataPtr);
      return true;
   }
}

FOSEGenericGraphNodeHandleCustomization::FOSEGenericGraphNodeHandleCustomization()
{
}

TSharedRef<IPropertyTypeCustomization> FOSEGenericGraphNodeHandleCustomization::MakeInstance()
{
   return MakeShareable(new FOSEGenericGraphNodeHandleCustomization);
}

void FOSEGenericGraphNodeHandleCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
   StructPropHandle = StructPropertyHandle;
   GraphTypeFilter = NAME_None;
   GraphFilterClass = nullptr;
   GraphFilterScriptStruct = nullptr;

   if (StructPropertyHandle->HasMetaData(TEXT("GraphType")))
   {
      const FString& GraphType = StructPropertyHandle->GetMetaData(TEXT("GraphType"));
      if (!GraphType.IsEmpty())
      {
         GraphTypeFilter = FName(*GraphType);
         if (GraphType.StartsWith(TEXT("/")))
         {
            GraphFilterClass = UClass::TryFindTypeSlow<UClass>(GraphType, EFindFirstObjectOptions::ExactClass);
         }
         if (GraphFilterClass == nullptr)
         {
            GraphFilterScriptStruct = UClass::TryFindTypeSlow<UScriptStruct>(GraphType);
         }
      }
   }

   FSimpleDelegate OnGraphChangedDelegate = FSimpleDelegate::CreateSP(this, &FOSEGenericGraphNodeHandleCustomization::OnGraphChanged);
   StructPropertyHandle->SetOnPropertyValueChanged(OnGraphChangedDelegate);

   HeaderRow
   .NameContent()
   [
      StructPropertyHandle->CreatePropertyNameWidget()
   ]
   .ValueContent()
   .VAlign(VAlign_Fill)
   [
      SNew(SBox)
      .Padding(FMargin(0.0f, 4.0f))
      .VAlign(VAlign_Center)
      [
         SNew(STextBlock)
         .Text(this, &FOSEGenericGraphNodeHandleCustomization::GetHeaderRowText)
         .Font(IDetailLayoutBuilder::GetDetailFont())
      ]
   ];
}

void FOSEGenericGraphNodeHandleCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
   GraphPropHandle = StructPropertyHandle->GetChildHandle("Graph");
   NodePropHandle = StructPropertyHandle->GetChildHandle("NodeId");
   IsSoftHandle = GraphPropHandle.IsValid() && CastField<FSoftObjectProperty>(GraphPropHandle->GetProperty()) != nullptr;

   if (GraphPropHandle.IsValid() && GraphPropHandle->IsValidHandle() && NodePropHandle.IsValid() && NodePropHandle->IsValidHandle())
   {
      FSimpleDelegate OnGraphChangedDelegate = FSimpleDelegate::CreateSP(this, &FOSEGenericGraphNodeHandleCustomization::OnGraphChanged);
      GraphPropHandle->SetOnPropertyValueChanged(OnGraphChangedDelegate);

      UOSEGenericGraph* CurrentGraph = TryGetCurrentGraphIfLoaded();
      RebuildNodeOptionList(CurrentGraph);

      TSharedPtr<FOSEGenericGraphNodeHandleComboBoxEntry> InitialValue;
      const FGuid CurrentNodeId = GetCurrentNodeId().Get(FGuid());
      for (const TSharedPtr<FOSEGenericGraphNodeHandleComboBoxEntry>& Value : NodeComboList)
      {
         if (Value->NodeId == CurrentNodeId)
         {
            InitialValue = Value;
            break;
         }
      }

      auto WrapInHorizontalBoxWithButtons = [this, &StructCustomizationUtils](TSharedRef<SWidget> InnerWidget) -> TSharedRef<SWidget>
      {
         TSharedRef<SHorizontalBox> Result = SNew(SHorizontalBox);

         Result->AddSlot()
            .AutoWidth()
            .MaxWidth(300.0f)
            [
               InnerWidget
            ];

         Result->AddSlot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
               SNew(SButton)
               .OnClicked(this, &FOSEGenericGraphNodeHandleCustomization::OnRefreshButtonClicked)
               .ToolTipText(LOCTEXT("GenericGraphNodeHandle_RefreshButton_Tooltip", "Reloads the list of nodes from the graph asset (eg. if it changed recently)"))
               [
                  SNew(STextBlock)
                  .Text(FEditorFontGlyphs::Refresh)
                  .Font(FAppStyle::Get().GetFontStyle("FontAwesome.9"))
               ]
            ];

         if (IsSoftHandle)
         {
            Result->AddSlot()
               .AutoWidth()
               .VAlign(VAlign_Center)
               [
                  SNew(SButton)
                  .OnClicked(this, &FOSEGenericGraphNodeHandleCustomization::OnSoftHandleLoadButtonClicked)
                  .IsEnabled(this, &FOSEGenericGraphNodeHandleCustomization::IsSoftHandleLoadButtonEnabled)
                  .ToolTipText(LOCTEXT("GenericGraphNodeHandle_SoftGraphLoadButton_Tooltip", "Loads the graph from the soft asset reference to resolve node names"))
                  [
                     SNew(STextBlock)
                     .Text(LOCTEXT("GenericGraphNodeHandle_SoftGraphLoadButton", "Load"))
                     .Font(StructCustomizationUtils.GetRegularFont())
                  ]
               ];
         }

         return Result;
      };

      // Construct a asset picker widget with a custom filter
      ChildBuilder.AddCustomRow(LOCTEXT("GenericGraphNodeHandle_GraphRowLabel", "Graph"))
         .NameContent()
         [
            SNew(STextBlock)
            .Text(LOCTEXT("GenericGraphNodeHandle_GraphRowLabel", "Graph"))
            .Font(StructCustomizationUtils.GetRegularFont())
         ]
         .ValueContent()
         .MaxDesiredWidth(0.0f)
         .VAlign(VAlign_Center)
         [
            SNew(SObjectPropertyEntryBox)
            .PropertyHandle(GraphPropHandle)
            .AllowedClass(UOSEGenericGraph::StaticClass())
            .OnShouldFilterAsset(this, &FOSEGenericGraphNodeHandleCustomization::ShouldFilterAsset)
         ];

      ChildBuilder.AddCustomRow(LOCTEXT("GenericGraphNodeHandle_NodeRowLabel", "Node"))
         .NameContent()
         [
            SNew(STextBlock)
            .Text(LOCTEXT("GenericGraphNodeHandle_NodeRowLabel", "Node"))
            .Font(StructCustomizationUtils.GetRegularFont())
         ]
         .ValueContent()
         .MaxDesiredWidth(0.0f)
         .VAlign(VAlign_Center)
         [
            WrapInHorizontalBoxWithButtons(
               SAssignNew(NodeComboBox, SComboBox<TSharedPtr<FOSEGenericGraphNodeHandleComboBoxEntry>>)
               .OptionsSource(&NodeComboList)
               .InitiallySelectedItem(InitialValue)
               .OnGenerateWidget(this, &FOSEGenericGraphNodeHandleCustomization::MakeNodeEntryWidget)
               .OnSelectionChanged(this, &FOSEGenericGraphNodeHandleCustomization::OnNodeChanged)
               .OnComboBoxOpening(this, &FOSEGenericGraphNodeHandleCustomization::OnOpenNodeSelectComboBox)
               .IsEnabled(FSlateApplication::Get().GetNormalExecutionAttribute())
               .ContentPadding(2)
               .Content()
               [
                  SNew(STextBlock)
                  .Text(this, &FOSEGenericGraphNodeHandleCustomization::GetCurrentNodeLabel)
                  .Font(IDetailLayoutBuilder::GetDetailFont())
               ]
            )
         ];
   }
}

TSharedRef<SWidget> FOSEGenericGraphNodeHandleCustomization::MakeNodeEntryWidget(TSharedPtr<FOSEGenericGraphNodeHandleComboBoxEntry> InItem)
{
   return
      SNew(STextBlock)
      .Text(InItem->Label)
      .Font(IDetailLayoutBuilder::GetDetailFont());
}

void FOSEGenericGraphNodeHandleCustomization::OnGraphChanged()
{
   UOSEGenericGraph* CurrentGraph = TryGetCurrentGraphIfLoaded();
   TOptional<FGuid> OldNodeValue = GetCurrentNodeId();

   // Clear name on graph change if no longer valid
   RebuildNodeOptionList(CurrentGraph);

   if (CurrentGraph == nullptr || !OldNodeValue || CurrentGraph->GetNodeByGuid(*OldNodeValue) == nullptr)
   {
      static const FGuid EmptyGuid{};
      SetCurrentNodeId(EmptyGuid);
   }
}

bool FOSEGenericGraphNodeHandleCustomization::ShouldFilterAsset(const FAssetData& AssetData)
{
   if (GraphTypeFilter.IsNone())
   {
      // No filter specified, allow all
      return false;
   }
   if (AssetData.AssetClassPath.GetAssetName() == GraphTypeFilter)
   {
      // Exact match for the filter
      return false;
   }
   if (GraphFilterClass != nullptr)
   {
      UClass* GraphClass = AssetData.GetClass(EResolveClass::No);
      if (GraphClass != nullptr && GraphClass->IsChildOf(GraphFilterClass))
      {
         return false;
      }
   }
   if (GraphFilterScriptStruct != nullptr)
   {
      // This is slow, but at the moment we don't have an alternative to the short struct name search
      UScriptStruct* GraphClass = UClass::TryFindTypeSlow<UScriptStruct>(AssetData.AssetClassPath.ToString());
      if (GraphClass != nullptr && GraphClass->IsChildOf(GraphFilterScriptStruct))
      {
         return false;
      }
   }
   return true;
}

UOSEGenericGraph* FOSEGenericGraphNodeHandleCustomization::TryGetCurrentGraphIfLoaded() const
{
   if (IsSoftHandle)
   {
      TSoftObjectPtr<UOSEGenericGraph> SoftGraph;
      if (GenericGraphHelpers::PropToSoftObjectPtr(GraphPropHandle, SoftGraph) && !SoftGraph.IsNull())
      {
         return SoftGraph.Get();
      }
   }
   else
   {
      UOSEGenericGraph* Graph = nullptr;
      if (GenericGraphHelpers::PropToObjectPtr(GraphPropHandle, Graph))
      {
         return Graph;
      }
   }
   return nullptr;
}

UOSEGenericGraph* FOSEGenericGraphNodeHandleCustomization::LoadCurrentGraphSynchronous() const
{
   if (IsSoftHandle)
   {
      TSoftObjectPtr<UOSEGenericGraph> SoftGraph;
      if (GenericGraphHelpers::PropToSoftObjectPtr(GraphPropHandle, SoftGraph) && !SoftGraph.IsNull())
      {
         return SoftGraph.LoadSynchronous();
      }
   }
   else
   {
      UOSEGenericGraph* Graph = nullptr;
      if (GenericGraphHelpers::PropToObjectPtr(GraphPropHandle, Graph))
      {
         return Graph;
      }
   }
   return nullptr;
}

TOptional<FGuid> FOSEGenericGraphNodeHandleCustomization::GetCurrentNodeId() const
{
   FGuid Result;
   if (GenericGraphHelpers::PropToGuid(NodePropHandle, Result))
   {
      return Result;
   }
   return NullOpt;
}

void FOSEGenericGraphNodeHandleCustomization::SetCurrentNodeId(const FGuid& NodeId)
{
   // Borrowed from Engine/Source/Editor/DetailCustomizations/Private/GuidStructCustomization.cpp
   auto WriteGuidToProperty = [](TSharedPtr<IPropertyHandle> GuidPropertyHandle, const FGuid& Guid)
   {
      FScopedTransaction Transaction(FText::Format(LOCTEXT("GenericGraphNodeHandle_EditGuidPropertyTransaction", "Edit {0}"), GuidPropertyHandle->GetPropertyDisplayName()));
      for (uint32 ChildIndex = 0; ChildIndex < 4; ++ChildIndex)
      {
         // Do not want a transaction on each individual set call as our scope transaction will handle it all
         // Need to mark first 3 as interactive so that post edit doesn't reinstance anything until we're done
         EPropertyValueSetFlags::Type GuidComponentFlags = EPropertyValueSetFlags::NotTransactable;
         TSharedRef<IPropertyHandle> ChildHandle = GuidPropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();
         ChildHandle->SetValue((int32)Guid[ChildIndex], ChildIndex != 3 ? GuidComponentFlags | EPropertyValueSetFlags::InteractiveChange : GuidComponentFlags);
      }
   };

   if (NodePropHandle.IsValid() && NodePropHandle->IsValidHandle())
   {
      WriteGuidToProperty(NodePropHandle, NodeId);
   }
}

void FOSEGenericGraphNodeHandleCustomization::OnNodeChanged(TSharedPtr<FOSEGenericGraphNodeHandleComboBoxEntry> NewSelection, ESelectInfo::Type SelectInfo)
{
   if (NewSelection.IsValid())
   {
      SetCurrentNodeId(NewSelection->NodeId);
   }
}

void FOSEGenericGraphNodeHandleCustomization::GetNodeOptions(UOSEGenericGraph* Graph, TArray<TSharedPtr<FOSEGenericGraphNodeHandleComboBoxEntry>>& OutEntries) const
{
   TSharedPtr<FOSEGenericGraphNodeHandleComboBoxEntry> SelectedItem;

   OutEntries.Reset();
   if (Graph != nullptr)
   {
      Graph->RebuildNodeIdMap();

      UOSEGenericGraphEdGraph* EdGraph = Cast<UOSEGenericGraphEdGraph>(Graph->EdGraph);
      if (EdGraph != nullptr && EdGraph->NodeMap.Num() == 0)
      {
         EdGraph->RebuildGenericGraph();
      }

      const TOptional<FGuid> CurrentNodeId = GetCurrentNodeId();

      for (const auto& Node : Graph->NodeIdMap)
      {
         if (Node.Value != nullptr)
         {
            TSharedPtr<FOSEGenericGraphNodeHandleComboBoxEntry> NewEntry = MakeShared<FOSEGenericGraphNodeHandleComboBoxEntry>();
            NewEntry->Label = Node.Value->GetNodeListViewTitle();
            NewEntry->NodeId = Node.Key;
            NewEntry->NodePos = GetNodePosition(EdGraph, Node.Value);
            OutEntries.Add(NewEntry);

            if (CurrentNodeId && NewEntry->NodeId == *CurrentNodeId)
            {
               SelectedItem = NewEntry;
            }
         }
      }

      // Sort the entries by graph position, then label
      OutEntries.Sort([](const TSharedPtr<FOSEGenericGraphNodeHandleComboBoxEntry>& A, const TSharedPtr<FOSEGenericGraphNodeHandleComboBoxEntry>& B)
      {
         // If we don't have a valid position, or these two nodes have the same position, then sort by label
         if (!A->NodePos || !B->NodePos || A->NodePos == B->NodePos)
         {
            return A->Label.CompareTo(B->Label) < 0;
         }

         // Same vertical position? Sort by horizontal position
         if (A->NodePos->Y == B->NodePos->Y)
         {
            return A->NodePos->X < B->NodePos->X;
         }

         // Sort by vertical position
         return A->NodePos->Y < B->NodePos->Y;
      });
   }

   {
      TSharedPtr<FOSEGenericGraphNodeHandleComboBoxEntry> NoneEntry = MakeShared<FOSEGenericGraphNodeHandleComboBoxEntry>();
      NoneEntry->Label = LOCTEXT("GenericGraphNode_NoneEntry", "None");
      NoneEntry->NodeId = FGuid();
      OutEntries.Insert(NoneEntry, 0);
   }

   if (NodeComboBox && SelectedItem)
   {
      NodeComboBox->SetSelectedItem(SelectedItem);
   }

   if (NodeComboBox)
   {
      NodeComboBox->RefreshOptions();
   }
}

void FOSEGenericGraphNodeHandleCustomization::RebuildNodeOptionList(UOSEGenericGraph* Graph)
{
   GetNodeOptions(Graph, NodeComboList);
}

void FOSEGenericGraphNodeHandleCustomization::OnOpenNodeSelectComboBox()
{
   // When opening the combo box, assume the user is explicitly asking for the node list, so always load synchronous
   GetNodeOptions(LoadCurrentGraphSynchronous(), NodeComboList);
}

FReply FOSEGenericGraphNodeHandleCustomization::OnRefreshButtonClicked()
{
   if (UOSEGenericGraph* Graph = TryGetCurrentGraphIfLoaded())
   {
      GetNodeOptions(Graph, NodeComboList);
   }
   return FReply::Handled();
}

FReply FOSEGenericGraphNodeHandleCustomization::OnSoftHandleLoadButtonClicked()
{
   if (!IsSoftHandle)
   {
      return FReply::Unhandled();
   }
   GetNodeOptions(LoadCurrentGraphSynchronous(), NodeComboList);
   return FReply::Handled();
}

bool FOSEGenericGraphNodeHandleCustomization::IsSoftHandleLoadButtonEnabled() const
{
   if (IsSoftHandle)
   {
      TSoftObjectPtr<UOSEGenericGraph> SoftGraph;
      if (GenericGraphHelpers::PropToSoftObjectPtr(GraphPropHandle, SoftGraph))
      {
         return !SoftGraph.IsNull() && !SoftGraph.IsValid();
      }
   }
   return false;
}

FText FOSEGenericGraphNodeHandleCustomization::GetHeaderRowText() const
{
   auto FormatHeaderRow = [](const FText& GraphLabel, const FText& NodeLabel) -> FText
   {
      return FText::FormatOrdered(INVTEXT("{0}: {1}"), GraphLabel, NodeLabel);
   };

   if (IsSoftHandle)
   {
      TSoftObjectPtr<UOSEGenericGraph> SoftGraph;
      if (GenericGraphHelpers::PropToSoftObjectPtr(GraphPropHandle, SoftGraph) && !SoftGraph.IsNull())
      {
         return FormatHeaderRow(FText::FromString(SoftGraph.GetAssetName()), GetCurrentNodeLabel());
      }
   }

   if (UOSEGenericGraph* Graph = TryGetCurrentGraphIfLoaded())
   {
      return FormatHeaderRow(FText::FromString(Graph->GetName()), GetCurrentNodeLabel());
   }

   return FText::GetEmpty();
}

FText FOSEGenericGraphNodeHandleCustomization::GetCurrentNodeLabel() const
{
   TOptional<FGuid> NodeGuid = GetCurrentNodeId();

   // Always show "None" if we don't have a valid guid
   if (!NodeGuid || !NodeGuid->IsValid())
   {
      return LOCTEXT("GenericGraphNode_NoneEntry", "None");
   }

   auto GuidToText = [](const FGuid& Guid) -> FText
   {
      return FText::FormatOrdered(LOCTEXT("GenericGraphNode_RawNodeGuidFormat", "Node Id {0}"), FText::FromString(Guid.ToString(EGuidFormats::DigitsWithHyphensLower)));
   };

   UOSEGenericGraph* Graph = TryGetCurrentGraphIfLoaded();

   // We have a non-zero guid and a null graph. Just show the raw guid.
   if (Graph == nullptr)
   {
      // Add an "unloaded" prefix if this is a soft handle
      static const FText UnloadedPrefix = LOCTEXT("GenericGraphNode_NotLoadedNodePrefix", "[NOT LOADED]");
      return IsSoftHandle
         ? FText::Join(INVTEXT(" "), UnloadedPrefix, GuidToText(*NodeGuid))
         : GuidToText(*NodeGuid);
   }

   UOSEGenericGraphNode* Node = Graph->GetNodeByGuid(*NodeGuid);
   return (Node != nullptr) ? Node->GetNodeListViewTitle() : GuidToText(*NodeGuid);
}

// static
TOptional<FIntVector2> FOSEGenericGraphNodeHandleCustomization::GetNodePosition(UOSEGenericGraphEdGraph* EdGraph, UOSEGenericGraphNode* Node)
{
   if (EdGraph == nullptr || Node == nullptr)
   {
      return NullOpt;
   }

   UOSEGenericGraphEdNode** EdNodePtr = EdGraph->NodeMap.Find(Node);
   UOSEGenericGraphEdNode* EdNode = (EdNodePtr != nullptr) ? *EdNodePtr : nullptr;
   if (EdNode == nullptr)
   {
      return NullOpt;
   }

   return FIntVector2{ EdNode->NodePosX, EdNode->NodePosY };
}

#undef LOCTEXT_NAMESPACE
