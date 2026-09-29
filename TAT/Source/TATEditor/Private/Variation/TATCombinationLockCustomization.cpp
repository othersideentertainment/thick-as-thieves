// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "TATCombinationLockCustomization.h"

// tat
#include "Lockpicking/TATCombinationScrape.h"
#include "Lockpicking/TATLockCombinationName.h"
#include "Quests/Modules/TATQuestGraph.h"

// ue5
#include "Editor/EditorEngine.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"

namespace LockCustomizationHelpers
{
   static const UTATQuestGraphBase* FindOwningQuestGraph(const IPropertyHandle& property)
   {
      TArray<UObject*> objects;
      property.GetOuterObjects(objects);
      for (const UObject* outer : objects)
      {
         // If in a node, the graph would be the outer of the outer
         if (const UTATQuestGraphBase* graph = Cast<UTATQuestGraphBase>(outer->GetOuter()))
         {
            return graph;
         }
      }

      return nullptr;
   }

   static const UWorld* GetWorldForQuestGraph(const UTATQuestGraphBase* questGraph)
   {
      return questGraph ? questGraph->Map.Get() : nullptr;
   }

   static void GetOptionStringsFromQuestGraph(const UTATQuestGraphBase* questGraph, TArray<TSharedPtr<FString>>& result)
   {
      const UWorld* world = GetWorldForQuestGraph(questGraph);
      if (world == nullptr)
      {
         return;
      }

      TSet<FName> names = CombinationScrape::GetLockCombinationNamesInWorld(world);
      result.Reserve(names.Num());

      for (const FName& name : names)
      {
         result.Emplace(MakeShared<FString>(name.ToString()));
      }
   }

   static void SelectLocksWithCombination(const UWorld* world, FName combination)
   {
      if (world == nullptr)
      {
         return;
      }
      TArray<UActorComponent*, TInlineAllocator<4>> components = CombinationScrape::FindLocksWithCombination(world, combination);

      GEditor->SelectNone(false, true);

      for (UActorComponent* component : components)
      {
         GEditor->SelectComponent(component, /*InSelected=*/true, /*bNotify=*/false, /*bSelectEvenIfHidden=*/true);
         GEditor->MoveViewportCamerasToActor(*component->GetOwner(), false);
      }

      GEditor->NoteSelectionChange();
   }

   static void OpenReferenceViewer(FName name)
   {
      if (FEditorDelegates::OnOpenReferenceViewer.IsBound() && !name.IsNone())
      {
         TArray<FAssetIdentifier> assetIdentifiers;
         assetIdentifiers.Emplace(FTATLockCombinationName::StaticStruct(), name);
         FEditorDelegates::OnOpenReferenceViewer.Broadcast(assetIdentifiers, FReferenceViewerParams());
      }
   }
}

void FTATLockCombinationNameCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> inStructPropertyHandle, class FDetailWidgetRow& headerRow, IPropertyTypeCustomizationUtils& structCustomizationUtils)
{
   TSharedPtr<IPropertyHandle> inner = inStructPropertyHandle->GetChildHandle(0);
   PropertyHandle = inner;
   
   auto findReferences = [this]()
      {
         if (PropertyHandle)
         {
            FName name;
            PropertyHandle->GetValue(name);
            LockCustomizationHelpers::OpenReferenceViewer(name);
         }
      };

   headerRow
      .NameContent()
      [
         inStructPropertyHandle->CreatePropertyNameWidget()
      ]
      .ValueContent()
      [
         SNew(SHorizontalBox)
         + SHorizontalBox::Slot()
         .AutoWidth()
         [
            inner->CreatePropertyValueWidget()
         ]
         + SHorizontalBox::Slot()
         .AutoWidth()
         [
            PropertyCustomizationHelpers::MakeBrowseButton(FSimpleDelegate::CreateLambda(findReferences),
               NSLOCTEXT("LockCombo", "FindReferences_Tooltip", "Find Asset References"),
               true,
               false)
         ]
      ];
}

void FTATLockCombinationNameRefCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> inStructPropertyHandle, class FDetailWidgetRow& headerRow, IPropertyTypeCustomizationUtils& structCustomizationUtils)
{
   TSharedPtr<IPropertyHandle> inner = inStructPropertyHandle->GetChildHandle(0);
   PropertyHandle = inner;

   const UTATQuestGraphBase* questGraph = LockCustomizationHelpers::FindOwningQuestGraph(*inStructPropertyHandle);
   TWeakObjectPtr<const UTATQuestGraphBase> weakQuestGraph = questGraph;
   OwningQuestGraph = weakQuestGraph;

   auto findReferences = [this]()
      {
         if (PropertyHandle)
         {
            FName name;
            PropertyHandle->GetValue(name);
            LockCustomizationHelpers::OpenReferenceViewer(name);
         }
      };

   auto selectActors = [this]()
      {
         if (PropertyHandle)
         {
            FName name;
            PropertyHandle->GetValue(name);
            const UWorld* world = LockCustomizationHelpers::GetWorldForQuestGraph(OwningQuestGraph.Get());
            LockCustomizationHelpers::SelectLocksWithCombination(world, name);
         }
      };

   FPropertyComboBoxArgs comboBoxArgs(inner, FOnGetPropertyComboBoxStrings::CreateLambda([weakQuestGraph](TArray<TSharedPtr<FString>>& outStrings, TArray<TSharedPtr<SToolTip>>& , TArray<bool>&) {
      LockCustomizationHelpers::GetOptionStringsFromQuestGraph(weakQuestGraph.Get(), outStrings);
   }));
   comboBoxArgs.ShowSearchForItemCount = 3;

   headerRow
      .NameContent()
      [
         inStructPropertyHandle->CreatePropertyNameWidget()
      ]
      .ValueContent()
      [
         // NOTE: using a widget switcher with a lambda effectively causes it to poll continuously
         //       I think this is acceptable here, but don't blindly copy without evaluating
         SNew(SWidgetSwitcher)
         .WidgetIndex_Lambda([weakQuestGraph]()
            {
               if (const UTATQuestGraphBase* questGraph = weakQuestGraph.Get())
               {
                  return questGraph->Map.IsValid() ? 1 : 0;
               }
               return 0;
            })
         + SWidgetSwitcher::Slot()
         [
            // If matching level not loaded, just show text field
            inner->CreatePropertyValueWidget()
         ]
         + SWidgetSwitcher::Slot()
         [
            // If it is loaded, show as full combo box
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
               PropertyCustomizationHelpers::MakePropertyComboBox(comboBoxArgs)
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
               PropertyCustomizationHelpers::MakeBrowseButton(FSimpleDelegate::CreateLambda(selectActors),
                  NSLOCTEXT("LockCombo", "SelectLocks_Tooltip", "Navigate to Actor in Level"),
                  true,
                  true)
            ]
         ]

      ];

   headerRow.AddCustomContextMenuAction(
      FUIAction(FExecuteAction::CreateLambda(findReferences)),
      NSLOCTEXT("LockCombo", "FindReferences_Tooltip", "Find Asset References"));
}
