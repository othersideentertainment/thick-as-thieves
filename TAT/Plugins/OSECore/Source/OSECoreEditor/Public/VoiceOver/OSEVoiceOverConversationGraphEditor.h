// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once


//ue4
#include "EditorUndoClient.h"
#include "Framework/Docking/TabManager.h"
#include "GraphEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/IToolkitHost.h"
#include "UObject/Object.h"

class UOSEVoiceOverConversation;
class UOSEVoiceOverConversationGraphNode;

class FOSEVoiceOverConversationGraphEditor : public FAssetEditorToolkit, public FGCObject, public FEditorUndoClient
{
public:
   virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
   virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;

   void Init(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UObject* ObjectToEdit);

   virtual ~FOSEVoiceOverConversationGraphEditor();

   /** FGCObject interface */
   virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
   virtual FString GetReferencerName() const override { return TEXT("OSEVoiceOverConversationGraphEditor"); }

   /** FAssetEditorToolkit interface */
   virtual FText GetBaseToolkitName() const override;
   virtual FName GetToolkitFName() const override;
   virtual FText GetToolkitName() const override;
   virtual FText GetToolkitToolTipText() const override;
   virtual FString GetWorldCentricTabPrefix() const override;

   /** @return Returns the color and opacity to use for the color that appears behind the tab text for this toolkit's tab in world-centric mode. */
   virtual FLinearColor GetWorldCentricTabColorScale() const override;

   void CreateConversationNode(UEdGraphPin* FromPin, FVector2D Location);

   /** FEditorUndoClient Interface */
   virtual void PostUndo(bool bSuccess) override;
   virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }

   /** Returns current graph handled by editor */
   UEdGraph* GetGraph();
private:
   TSharedRef<SDockTab> SpawnTab_GraphCanvas(const FSpawnTabArgs& Args);
   TSharedRef<SDockTab> SpawnTab_Properties(const FSpawnTabArgs& Args);

   /** Creates all internal widgets for the tabs to point at */
   void CreateInternalWidgets(UOSEVoiceOverConversation* conversation);

   /** Create new graph editor widget */
   TSharedRef<SGraphEditor> CreateGraphEditorWidget(UOSEVoiceOverConversation* conversation);

   /** Called when the selection changes in the GraphEditor */
   void OnSelectedNodesChanged(const TSet<UObject*>& NewSelection);


   /** Command Handlers */
   void _SelectAllNodes();
   bool _CanSelectAllNodes() const;

   void _RemoveSelectedNodes();
   bool _CanRemoveNodes() const;

   void _CutSelectedNodes();
   bool _CanCutNodes() const;

   void _CopySelectedNodes();
   bool _CanCopyNodes() const;

   void _PasteNodes();
   void _PasteNodesHere(const FVector2D& Location);
   bool _CanPasteNodes() const;

   void _DuplicateNodes();
   bool _CanDuplicateNodes() const;

   void _SetupGraphCommands();

   void UndoGraphAction();
   void RedoGraphAction();

   //Helpers

   void _RemoveNode(UOSEVoiceOverConversationGraphNode& node);



   /** Graph Editor */
   TSharedPtr<SGraphEditor> GraphEditor;

   /** Property View */
   TSharedPtr<IDetailsView> DetailsView;

   /** Command list for this editor */
   TSharedPtr<FUICommandList> GraphEditorCommands;

   /**   The tab ids for all the tabs used */
   static const FName GraphCanvasTabId;
   static const FName PropertiesTabId;
};

