// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#pragma once

#include "CoreMinimal.h"
#include "GraphEditor.h"
#include "OSEGenericGraphEditorSettings.h"
#include "OSEGenericGraph.h"
#include "Toolkits/AssetEditorToolkit.h"

#if ENGINE_MAJOR_VERSION == 5
#include "UObject/ObjectSaveContext.h"
#endif // #if ENGINE_MAJOR_VERSION == 5

class FOSEGenericGraphAssetEditorToolbar;

class OSEGENERICGRAPHEDITOR_API FOSEGenericGraphAssetEditor : public FAssetEditorToolkit, public FNotifyHook, public FGCObject
{
public:
   FOSEGenericGraphAssetEditor();
   virtual ~FOSEGenericGraphAssetEditor();

   void InitGenericGraphAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UOSEGenericGraph* Graph);

   // IToolkit interface
   virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
   virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
   // End of IToolkit interface

   // FAssetEditorToolkit
   virtual FName GetToolkitFName() const override;
   virtual FText GetBaseToolkitName() const override;
   virtual FText GetToolkitName() const override;
   virtual FText GetToolkitToolTipText() const override;
   virtual FLinearColor GetWorldCentricTabColorScale() const override;
   virtual FString GetWorldCentricTabPrefix() const override;
   virtual FString GetDocumentationLink() const override;
   virtual void SaveAsset_Execute() override;
   // End of FAssetEditorToolkit

   //Toolbar
   void UpdateToolbar();
   TSharedPtr<FOSEGenericGraphAssetEditorToolbar> GetToolbarBuilder() { return ToolbarBuilder; }
   void RegisterToolbarTab(const TSharedRef<class FTabManager>& TabManager);


   // FSerializableObject interface
   virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
   // End of FSerializableObject interface

#if ENGINE_MAJOR_VERSION == 5
   // FGCObject interface
   virtual FString GetReferencerName() const
   {
      return TEXT("FOSEGenericGraphAssetEditor");
   }
   // ~FGCObject interface
#endif // #if ENGINE_MAJOR_VERSION == 5

   const FOSEGenericGraphEditorSettings& GetSettings() const { return GenericGraphEditorSettings; }

protected:
   TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);
   TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);
   TSharedRef<SDockTab> SpawnTab_EditorSettings(const FSpawnTabArgs& Args);

   void CreateInternalWidgets();
   TSharedRef<SGraphEditor> CreateViewportWidget();


   void BindCommands();

   void CreateEdGraph();

   void CreateCommandList();

   TSharedPtr<SGraphEditor> GetCurrGraphEditor() const;

   FGraphPanelSelectionSet GetSelectedNodes() const;

   void RebuildGenericGraph();

   // Delegates for graph editor commands
   void SelectAllNodes();
   bool CanSelectAllNodes();
   void DeleteSelectedNodes();
   bool CanDeleteNodes();
   void DeleteSelectedDuplicatableNodes();
   void CutSelectedNodes();
   bool CanCutNodes();
   void CopySelectedNodes();
   bool CanCopyNodes();
   void PasteNodes();
   void PasteNodesHere(const FVector2D& Location);
   bool CanPasteNodes();
   void DuplicateNodes();
   bool CanDuplicateNodes();

   void GraphSettings();
   bool CanGraphSettings() const;

   void AutoArrange();
   bool CanAutoArrange() const;

   void RefreshGraph();
   bool CanRefreshGraph() const;

   void OnRenameNode();
   bool CanRenameNodes() const;

   // Property editor delegates
   bool IsGraphPropertyVisible(const FPropertyAndParent& Prop) const;

   //////////////////////////////////////////////////////////////////////////
   // graph editor event
   void OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection);

   void OnNodeDoubleClicked(UEdGraphNode* Node);

   void OnNodeTextCommitted(const FText& NewText, ETextCommit::Type CommitType, UEdGraphNode* Node);

   FReply OnSpawnNodeByShortcut(FInputChord Chord, const FVector2D& Location);

   void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);

#if ENGINE_MAJOR_VERSION < 5
   void OnPackageSaved(const FString& PackageFileName, UObject* Outer);
#else // #if ENGINE_MAJOR_VERSION < 5
   void OnPackageSavedWithContext(const FString& PackageFileName, UPackage* Package, FObjectPostSaveContext ObjectSaveContext);
#endif // #else // #if ENGINE_MAJOR_VERSION < 5

protected:
   FOSEGenericGraphEditorSettings GenericGraphEditorSettings;

   TObjectPtr<UOSEGenericGraph> EditingGraph;

   //Toolbar
   TSharedPtr<FOSEGenericGraphAssetEditorToolbar> ToolbarBuilder;

   /** Handle to the registered OnPackageSave delegate */
   FDelegateHandle OnPackageSavedDelegateHandle;

   TSharedPtr<SGraphEditor> ViewportWidget;
   TSharedPtr<class IDetailsView> PropertyWidget;
   TSharedPtr<class IStructureDetailsView> EditorSettingsWidget;

   /** The command list for this editor */
   TSharedPtr<FUICommandList> GraphEditorCommands;
};


