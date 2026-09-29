// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#include "OSEGenericGraphEditor.h"
#include "OSEGenericGraphNodeFactory.h"
#include "OSEGenericGraphAssetTypeActions.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEditorStyle.h"
#include "OSEGenericGraphNodeHandle.h"
#include "OSEGenericGraphSoftNodeHandle.h"
#include "OSEGenericGraphNodeHandleCustomization.h"
#include "PropertyEditorModule.h"
#include "OSEGenericGraph.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEdGraph.h"

#include "UObject/ObjectSaveContext.h"

DEFINE_LOG_CATEGORY(OSEGenericGraphEditor)

#define LOCTEXT_NAMESPACE "OSEGenericGraphEditor"

void FOSEGenericGraphEditor::StartupModule()
{
   FOSEGenericGraphEditorStyle::Initialize();

   GraphPanelNodeFactory_GenericGraph = MakeShareable(new FOSEGenericGraphPanelNodeFactory());
   FEdGraphUtilities::RegisterVisualNodeFactory(GraphPanelNodeFactory_GenericGraph);

   IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

   GenericGraphAssetCategoryBit = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("GenericGraph")), LOCTEXT("GenericGraphAssetCategory", "GenericGraph"));

   RegisterAssetTypeAction(AssetTools, MakeShareable(new FOSEGenericGraphAssetTypeActions(GenericGraphAssetCategoryBit)));

   FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
   PropertyModule.RegisterCustomPropertyTypeLayout(FOSEGenericGraphNodeHandle::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FOSEGenericGraphNodeHandleCustomization::MakeInstance));
   PropertyModule.RegisterCustomPropertyTypeLayout(FOSEGenericGraphSoftNodeHandle::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FOSEGenericGraphNodeHandleCustomization::MakeInstance));

   UPackage::PreSavePackageWithContextEvent.AddRaw(this, &FOSEGenericGraphEditor::HandlePreSavePackage);
}

void FOSEGenericGraphEditor::ShutdownModule()
{
   UPackage::PreSavePackageWithContextEvent.RemoveAll(this);

   FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
   PropertyModule.UnregisterCustomPropertyTypeLayout(FOSEGenericGraphNodeHandle::StaticStruct()->GetFName());
   PropertyModule.UnregisterCustomPropertyTypeLayout(FOSEGenericGraphSoftNodeHandle::StaticStruct()->GetFName());

   // Unregister all the asset types that we registered
   if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
   {
      IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
      for (int32 Index = 0; Index < CreatedAssetTypeActions.Num(); ++Index)
      {
         AssetTools.UnregisterAssetTypeActions(CreatedAssetTypeActions[Index].ToSharedRef());
      }
   }

   if (GraphPanelNodeFactory_GenericGraph.IsValid())
   {
      FEdGraphUtilities::UnregisterVisualNodeFactory(GraphPanelNodeFactory_GenericGraph);
      GraphPanelNodeFactory_GenericGraph.Reset();
   }

   FOSEGenericGraphEditorStyle::Shutdown();
}

void FOSEGenericGraphEditor::RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
{
   AssetTools.RegisterAssetTypeActions(Action);
   CreatedAssetTypeActions.Add(Action);
}

void FOSEGenericGraphEditor::HandlePreSavePackage(UPackage* Package, FObjectPreSaveContext ObjectSaveContext)
{
   // Rebuild generic graph if saving package with it
   // TODO: Not ideal to do this here, as it will run on every package save. An alternative could
   //       to have a PreSaveRoot on the GenericGraph asset that triggers a rebuild via a static delegate
   //       that this module would hook to. (It can't do it directly, because of module dependencies)
   //       I was hesitant to do this in pre-save on the UOSEGenericGraphEdGraph itself.

   // Skip when cooking just to be conservative, and avoid changes in contents during cook
   // Since rebuild is not deterministic in terms of the object names
   if (ObjectSaveContext.IsCooking())
   {
      return;
   }

   constexpr bool bIncludeNestedObjects = false;
   TArray<UOSEGenericGraphEdGraph*> EdGraphs;
   ForEachObjectWithPackage(Package, [&EdGraphs](UObject* RootPackageObject) {
      if (UOSEGenericGraph* GenericGraph = Cast<UOSEGenericGraph>(RootPackageObject))
      {
         if (UOSEGenericGraphEdGraph* EdGraph = Cast<UOSEGenericGraphEdGraph>(GenericGraph->EdGraph))
         {
            EdGraphs.Add(EdGraph);
         }
      }
      return true;
      }, bIncludeNestedObjects);
   for (UOSEGenericGraphEdGraph* EdGraph : EdGraphs)
   {
      EdGraph->RebuildGenericGraph();
   }
}

IMPLEMENT_MODULE(FOSEGenericGraphEditor, OSEGenericGraphEditor)

#undef LOCTEXT_NAMESPACE

