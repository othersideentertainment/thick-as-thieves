// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#include "GenericGraphAssetEditor/OSEGenericGraphAssetEditor.h"
#include "OSEGenericGraphEditorPCH.h"
#include "GenericGraphAssetEditor/OSEGenericGraphAssetEditorToolbar.h"
#include "GenericGraphAssetEditor/OSEGenericGraphAssetSchema.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEditorCommands.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEdGraph.h"
#include "AssetToolsModule.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Framework/Commands/GenericCommands.h"
#include "GraphEditorActions.h"
#include "IDetailsView.h"
#include "IStructureDetailsView.h"
#include "PropertyEditorModule.h"
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EdGraphUtilities.h"
#include "ScopedTransaction.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEdGraph.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEdNode.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEdNodeEdge.h"
#include "AutoLayout/OSETreeLayoutStrategy.h"
#include "AutoLayout/OSEForceDirectedLayoutStrategy.h"

#define LOCTEXT_NAMESPACE "OSEGenericGraphAssetEditor"

const FName GenericGraphEditorAppName = FName(TEXT("OSEGenericGraphEditorApp"));

struct FGenericGraphAssetEditorTabs
{
   // Tab identifiers
   static const FName GenericGraphPropertyID;
   static const FName ViewportID;
   static const FName GenericGraphEditorSettingsID;
};

//////////////////////////////////////////////////////////////////////////

const FName FGenericGraphAssetEditorTabs::GenericGraphPropertyID(TEXT("OSEGenericGraphProperty"));
const FName FGenericGraphAssetEditorTabs::ViewportID(TEXT("Viewport"));
const FName FGenericGraphAssetEditorTabs::GenericGraphEditorSettingsID(TEXT("OSEGenericGraphEditorSettings"));

//////////////////////////////////////////////////////////////////////////

FOSEGenericGraphAssetEditor::FOSEGenericGraphAssetEditor()
{
   EditingGraph = nullptr;

#if 0 // Doing this elsewhere
#if ENGINE_MAJOR_VERSION < 5
   OnPackageSavedDelegateHandle = UPackage::PackageSavedEvent.AddRaw(this, &FOSEGenericGraphAssetEditor::OnPackageSaved);
#else // #if ENGINE_MAJOR_VERSION < 5
   OnPackageSavedDelegateHandle = UPackage::PackageSavedWithContextEvent.AddRaw(this, &FOSEGenericGraphAssetEditor::OnPackageSavedWithContext);
#endif // #else // #if ENGINE_MAJOR_VERSION < 5
#endif
}

FOSEGenericGraphAssetEditor::~FOSEGenericGraphAssetEditor()
{
#if 0 // Doing this elsewhere
#if ENGINE_MAJOR_VERSION < 5
   UPackage::PackageSavedEvent.Remove(OnPackageSavedDelegateHandle);
#else // #if ENGINE_MAJOR_VERSION < 5
   UPackage::PackageSavedWithContextEvent.Remove(OnPackageSavedDelegateHandle);
#endif // #else // #if ENGINE_MAJOR_VERSION < 5
#endif
}

void FOSEGenericGraphAssetEditor::InitGenericGraphAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UOSEGenericGraph* Graph)
{
   EditingGraph = Graph;
   CreateEdGraph();

   FGenericCommands::Register();
   FGraphEditorCommands::Register();
   FOSEGenericGraphEditorCommands::Register();

   if (!ToolbarBuilder.IsValid())
   {
      ToolbarBuilder = MakeShareable(new FOSEGenericGraphAssetEditorToolbar(SharedThis(this)));
   }

   BindCommands();

   CreateInternalWidgets();

   TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

   ToolbarBuilder->AddGenericGraphToolbar(ToolbarExtender);

   // Layout
   const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("OSEGenericGraphEditor_Layout_v1")
      ->AddArea
      (
         FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
#if ENGINE_MAJOR_VERSION < 5
         ->Split
         (
            FTabManager::NewStack()
            ->SetSizeCoefficient(0.1f)
            ->AddTab(GetToolbarTabId(), ETabState::OpenedTab)->SetHideTabWell(true)
         )
#endif // #if ENGINE_MAJOR_VERSION < 5
         ->Split
         (
            FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)->SetSizeCoefficient(0.9f)
            ->Split
            (
               FTabManager::NewStack()
               ->SetSizeCoefficient(0.65f)
               ->AddTab(FGenericGraphAssetEditorTabs::ViewportID, ETabState::OpenedTab)->SetHideTabWell(true)
            )
            ->Split
            (
               FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
               ->Split
               (
                  FTabManager::NewStack()
                  ->SetSizeCoefficient(0.7f)
                  ->AddTab(FGenericGraphAssetEditorTabs::GenericGraphPropertyID, ETabState::OpenedTab)->SetHideTabWell(true)
               )
               ->Split
               (
                  FTabManager::NewStack()
                  ->SetSizeCoefficient(0.3f)
                  ->AddTab(FGenericGraphAssetEditorTabs::GenericGraphEditorSettingsID, ETabState::OpenedTab)
               )
            )
         )
      );

   const bool bCreateDefaultStandaloneMenu = true;
   const bool bCreateDefaultToolbar = true;
   FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, GenericGraphEditorAppName, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, EditingGraph, false);

   RegenerateMenusAndToolbars();
}

void FOSEGenericGraphAssetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
   WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_OSEGenericGraphEditor", "OSE Generic Graph Editor"));
   auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

   FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

   InTabManager->RegisterTabSpawner(FGenericGraphAssetEditorTabs::ViewportID, FOnSpawnTab::CreateSP(this, &FOSEGenericGraphAssetEditor::SpawnTab_Viewport))
      .SetDisplayName(LOCTEXT("GraphCanvasTab", "Viewport"))
      .SetGroup(WorkspaceMenuCategoryRef)
      .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x"));

   InTabManager->RegisterTabSpawner(FGenericGraphAssetEditorTabs::GenericGraphPropertyID, FOnSpawnTab::CreateSP(this, &FOSEGenericGraphAssetEditor::SpawnTab_Details))
      .SetDisplayName(LOCTEXT("DetailsTab", "Properties"))
      .SetGroup(WorkspaceMenuCategoryRef)
      .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

   InTabManager->RegisterTabSpawner(FGenericGraphAssetEditorTabs::GenericGraphEditorSettingsID, FOnSpawnTab::CreateSP(this, &FOSEGenericGraphAssetEditor::SpawnTab_EditorSettings))
      .SetDisplayName(LOCTEXT("EditorSettingsTab", "Graph Editor Setttings"))
      .SetGroup(WorkspaceMenuCategoryRef)
      .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FOSEGenericGraphAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
   FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

   InTabManager->UnregisterTabSpawner(FGenericGraphAssetEditorTabs::ViewportID);
   InTabManager->UnregisterTabSpawner(FGenericGraphAssetEditorTabs::GenericGraphPropertyID);
   InTabManager->UnregisterTabSpawner(FGenericGraphAssetEditorTabs::GenericGraphEditorSettingsID);
}

FName FOSEGenericGraphAssetEditor::GetToolkitFName() const
{
   return FName("FOSEGenericGraphAssetEditor");
}

FText FOSEGenericGraphAssetEditor::GetBaseToolkitName() const
{
   return LOCTEXT("GenericGraphEditorAppLabel", "OSE Generic Graph Editor");
}

FText FOSEGenericGraphAssetEditor::GetToolkitName() const
{
   const bool bDirtyState = EditingGraph->GetOutermost()->IsDirty();

   FFormatNamedArguments Args;
   Args.Add(TEXT("GenericGraphName"), FText::FromString(EditingGraph->GetName()));
   Args.Add(TEXT("DirtyState"), bDirtyState ? FText::FromString(TEXT("*")) : FText::GetEmpty());
   return FText::Format(LOCTEXT("GenericGraphEditorToolkitName", "{GenericGraphName}{DirtyState}"), Args);
}

FText FOSEGenericGraphAssetEditor::GetToolkitToolTipText() const
{
   return FAssetEditorToolkit::GetToolTipTextForObject(EditingGraph);
}

FLinearColor FOSEGenericGraphAssetEditor::GetWorldCentricTabColorScale() const
{
   return FLinearColor::White;
}

FString FOSEGenericGraphAssetEditor::GetWorldCentricTabPrefix() const
{
   return TEXT("OSEGenericGraphEditor");
}

FString FOSEGenericGraphAssetEditor::GetDocumentationLink() const
{
   return TEXT("");
}

void FOSEGenericGraphAssetEditor::SaveAsset_Execute()
{
   if (EditingGraph != nullptr)
   {
      RebuildGenericGraph();
   }

   FAssetEditorToolkit::SaveAsset_Execute();
}

void FOSEGenericGraphAssetEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
   Collector.AddReferencedObject(EditingGraph);
   Collector.AddReferencedObject(EditingGraph->EdGraph);
}

TSharedRef<SDockTab> FOSEGenericGraphAssetEditor::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
   check(Args.GetTabId() == FGenericGraphAssetEditorTabs::ViewportID);

   TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
      .Label(LOCTEXT("ViewportTab_Title", "Viewport"));

   if (ViewportWidget.IsValid())
   {
      SpawnedTab->SetContent(ViewportWidget.ToSharedRef());
   }

   return SpawnedTab;
}

TSharedRef<SDockTab> FOSEGenericGraphAssetEditor::SpawnTab_Details(const FSpawnTabArgs& Args)
{
   check(Args.GetTabId() == FGenericGraphAssetEditorTabs::GenericGraphPropertyID);

   return SNew(SDockTab)
#if ENGINE_MAJOR_VERSION < 5
      .Icon(FAppStyle::GetBrush("LevelEditor.Tabs.Details"))
#endif // #if ENGINE_MAJOR_VERSION < 5
      .Label(LOCTEXT("Details_Title", "Properties"))
      [
         PropertyWidget.ToSharedRef()
      ];
}

TSharedRef<SDockTab> FOSEGenericGraphAssetEditor::SpawnTab_EditorSettings(const FSpawnTabArgs& Args)
{
   check(Args.GetTabId() == FGenericGraphAssetEditorTabs::GenericGraphEditorSettingsID);

   return SNew(SDockTab)
#if ENGINE_MAJOR_VERSION < 5
      .Icon(FAppStyle::GetBrush("LevelEditor.Tabs.Details"))
#endif // #if ENGINE_MAJOR_VERSION < 5
      .Label(LOCTEXT("EditorSettings_Title", "Generic Graph Editor Setttings"))
      [
         EditorSettingsWidget->GetWidget().ToSharedRef()
      ];
}

void FOSEGenericGraphAssetEditor::CreateInternalWidgets()
{
   ViewportWidget = CreateViewportWidget();

   FDetailsViewArgs Args;
   Args.bHideSelectionTip = true;
   Args.NotifyHook = this;

   FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
   PropertyWidget = PropertyModule.CreateDetailView(Args);
   PropertyWidget->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &FOSEGenericGraphAssetEditor::IsGraphPropertyVisible));
   PropertyWidget->OnFinishedChangingProperties().AddSP(this, &FOSEGenericGraphAssetEditor::OnFinishedChangingProperties);
   PropertyWidget->SetObject(EditingGraph);

   FStructureDetailsViewArgs StructArgs{};
   EditorSettingsWidget = PropertyModule.CreateStructureDetailView(Args, StructArgs,
      MakeShared<FStructOnScope>(FOSEGenericGraphEditorSettings::StaticStruct(), (uint8*)&GenericGraphEditorSettings));
   EditorSettingsWidget->GetOnFinishedChangingPropertiesDelegate().AddLambda([this](const FPropertyChangedEvent& Prop)
   {
      if (PropertyWidget.IsValid())
      {
         PropertyWidget->ForceRefresh();
      }
   });
}

TSharedRef<SGraphEditor> FOSEGenericGraphAssetEditor::CreateViewportWidget()
{
   FGraphAppearanceInfo AppearanceInfo;
   AppearanceInfo.CornerText = EditingGraph->GetGraphTypeDisplayName();

   CreateCommandList();

   SGraphEditor::FGraphEditorEvents InEvents;
   InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FOSEGenericGraphAssetEditor::OnSelectedNodesChanged);
   InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FOSEGenericGraphAssetEditor::OnNodeDoubleClicked);
   InEvents.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &FOSEGenericGraphAssetEditor::OnNodeTextCommitted);
   InEvents.OnSpawnNodeByShortcut = SGraphEditor::FOnSpawnNodeByShortcut::CreateSP(this, &FOSEGenericGraphAssetEditor::OnSpawnNodeByShortcut);

   return SNew(SGraphEditor)
      .AdditionalCommands(GraphEditorCommands)
      .IsEditable(true)
      .Appearance(AppearanceInfo)
      .GraphToEdit(EditingGraph->EdGraph)
      .GraphEvents(InEvents)
      .AutoExpandActionMenu(true)
      .ShowGraphStateOverlay(false);
}

void FOSEGenericGraphAssetEditor::BindCommands()
{
   ToolkitCommands->MapAction(FOSEGenericGraphEditorCommands::Get().GraphSettings,
      FExecuteAction::CreateSP(this, &FOSEGenericGraphAssetEditor::GraphSettings),
      FCanExecuteAction::CreateSP(this, &FOSEGenericGraphAssetEditor::CanGraphSettings)
   );

   ToolkitCommands->MapAction(FOSEGenericGraphEditorCommands::Get().Refresh,
      FExecuteAction::CreateSP(this, &FOSEGenericGraphAssetEditor::RefreshGraph),
      FCanExecuteAction::CreateSP(this, &FOSEGenericGraphAssetEditor::CanRefreshGraph)
   );

   ToolkitCommands->MapAction(FOSEGenericGraphEditorCommands::Get().AutoArrange,
      FExecuteAction::CreateSP(this, &FOSEGenericGraphAssetEditor::AutoArrange),
      FCanExecuteAction::CreateSP(this, &FOSEGenericGraphAssetEditor::CanAutoArrange)
   );
}

void FOSEGenericGraphAssetEditor::CreateEdGraph()
{
   if (EditingGraph->EdGraph == nullptr)
   {
      EditingGraph->EdGraph = CastChecked<UOSEGenericGraphEdGraph>(FBlueprintEditorUtils::CreateNewGraph(EditingGraph, NAME_None, UOSEGenericGraphEdGraph::StaticClass(), UOSEGenericGraphAssetSchema::StaticClass()));
      EditingGraph->EdGraph->bAllowDeletion = false;

      // Give the schema a chance to fill out any required nodes (like the results node)
      const UEdGraphSchema* Schema = EditingGraph->EdGraph->GetSchema();
      Schema->CreateDefaultNodesForGraph(*EditingGraph->EdGraph);
   }
}

void FOSEGenericGraphAssetEditor::CreateCommandList()
{
   if (GraphEditorCommands.IsValid())
   {
      return;
   }

   GraphEditorCommands = MakeShareable(new FUICommandList);

   // Can't use CreateSP here because derived editor are already implementing TSharedFromThis<FAssetEditorToolkit>
   // however it should be safe, since commands are being used only within this editor
   // if it ever crashes, this function will have to go away and be reimplemented in each derived class

   GraphEditorCommands->MapAction(FOSEGenericGraphEditorCommands::Get().GraphSettings,
      FExecuteAction::CreateRaw(this, &FOSEGenericGraphAssetEditor::GraphSettings),
      FCanExecuteAction::CreateRaw(this, &FOSEGenericGraphAssetEditor::CanGraphSettings));

   GraphEditorCommands->MapAction(FOSEGenericGraphEditorCommands::Get().AutoArrange,
      FExecuteAction::CreateRaw(this, &FOSEGenericGraphAssetEditor::AutoArrange),
      FCanExecuteAction::CreateRaw(this, &FOSEGenericGraphAssetEditor::CanAutoArrange));

   GraphEditorCommands->MapAction(FGenericCommands::Get().SelectAll,
      FExecuteAction::CreateRaw(this, &FOSEGenericGraphAssetEditor::SelectAllNodes),
      FCanExecuteAction::CreateRaw(this, &FOSEGenericGraphAssetEditor::CanSelectAllNodes)
   );

   GraphEditorCommands->MapAction(FGenericCommands::Get().Delete,
      FExecuteAction::CreateRaw(this, &FOSEGenericGraphAssetEditor::DeleteSelectedNodes),
      FCanExecuteAction::CreateRaw(this, &FOSEGenericGraphAssetEditor::CanDeleteNodes)
   );

   GraphEditorCommands->MapAction(FGenericCommands::Get().Copy,
      FExecuteAction::CreateRaw(this, &FOSEGenericGraphAssetEditor::CopySelectedNodes),
      FCanExecuteAction::CreateRaw(this, &FOSEGenericGraphAssetEditor::CanCopyNodes)
   );

   GraphEditorCommands->MapAction(FGenericCommands::Get().Cut,
      FExecuteAction::CreateRaw(this, &FOSEGenericGraphAssetEditor::CutSelectedNodes),
      FCanExecuteAction::CreateRaw(this, &FOSEGenericGraphAssetEditor::CanCutNodes)
   );

   GraphEditorCommands->MapAction(FGenericCommands::Get().Paste,
      FExecuteAction::CreateRaw(this, &FOSEGenericGraphAssetEditor::PasteNodes),
      FCanExecuteAction::CreateRaw(this, &FOSEGenericGraphAssetEditor::CanPasteNodes)
   );

   GraphEditorCommands->MapAction(FGenericCommands::Get().Duplicate,
      FExecuteAction::CreateRaw(this, &FOSEGenericGraphAssetEditor::DuplicateNodes),
      FCanExecuteAction::CreateRaw(this, &FOSEGenericGraphAssetEditor::CanDuplicateNodes)
   );

   GraphEditorCommands->MapAction(FGenericCommands::Get().Rename,
      FExecuteAction::CreateSP(this, &FOSEGenericGraphAssetEditor::OnRenameNode),
      FCanExecuteAction::CreateSP(this, &FOSEGenericGraphAssetEditor::CanRenameNodes)
   );
}

TSharedPtr<SGraphEditor> FOSEGenericGraphAssetEditor::GetCurrGraphEditor() const
{
   return ViewportWidget;
}

FGraphPanelSelectionSet FOSEGenericGraphAssetEditor::GetSelectedNodes() const
{
   FGraphPanelSelectionSet CurrentSelection;
   TSharedPtr<SGraphEditor> FocusedGraphEd = GetCurrGraphEditor();
   if (FocusedGraphEd.IsValid())
   {
      CurrentSelection = FocusedGraphEd->GetSelectedNodes();
   }

   return CurrentSelection;
}

void FOSEGenericGraphAssetEditor::RebuildGenericGraph()
{
   if (EditingGraph == nullptr)
   {
      LOG_WARNING(TEXT("FOSEGenericGraphAssetEditor::RebuildGenericGraph EditingGraph is nullptr"));
      return;
   }

   UOSEGenericGraphEdGraph* EdGraph = Cast<UOSEGenericGraphEdGraph>(EditingGraph->EdGraph);
   check(EdGraph != nullptr);

   EdGraph->RebuildGenericGraph();
}

void FOSEGenericGraphAssetEditor::SelectAllNodes()
{
   TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
   if (CurrentGraphEditor.IsValid())
   {
      CurrentGraphEditor->SelectAllNodes();
   }
}

bool FOSEGenericGraphAssetEditor::CanSelectAllNodes()
{
   return true;
}

void FOSEGenericGraphAssetEditor::DeleteSelectedNodes()
{
   TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
   if (!CurrentGraphEditor.IsValid())
   {
      return;
   }

   const FScopedTransaction Transaction(FGenericCommands::Get().Delete->GetDescription());

   CurrentGraphEditor->GetCurrentGraph()->Modify();

   const FGraphPanelSelectionSet SelectedNodes = CurrentGraphEditor->GetSelectedNodes();
   CurrentGraphEditor->ClearSelectionSet();

   for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
   {
      UEdGraphNode* EdNode = Cast<UEdGraphNode>(*NodeIt);
      if (EdNode == nullptr || !EdNode->CanUserDeleteNode())
         continue;;

      if (UOSEGenericGraphEdNode* EdNode_Node = Cast<UOSEGenericGraphEdNode>(EdNode))
      {
         EdNode_Node->Modify();

         const UEdGraphSchema* Schema = EdNode_Node->GetSchema();
         if (Schema != nullptr)
         {
            Schema->BreakNodeLinks(*EdNode_Node);
         }

         EdNode_Node->DestroyNode();
      }
      else
      {
         EdNode->Modify();
         EdNode->DestroyNode();
      }
   }
}

bool FOSEGenericGraphAssetEditor::CanDeleteNodes()
{
   // If any of the nodes can be deleted then we should allow deleting
   const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
   for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
   {
      UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
      if (Node != nullptr && Node->CanUserDeleteNode())
      {
         return true;
      }
   }

   return false;
}

void FOSEGenericGraphAssetEditor::DeleteSelectedDuplicatableNodes()
{
   TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
   if (!CurrentGraphEditor.IsValid())
   {
      return;
   }

   const FGraphPanelSelectionSet OldSelectedNodes = CurrentGraphEditor->GetSelectedNodes();
   CurrentGraphEditor->ClearSelectionSet();

   for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
   {
      UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
      if (Node && Node->CanDuplicateNode())
      {
         CurrentGraphEditor->SetNodeSelection(Node, true);
      }
   }

   // Delete the duplicatable nodes
   DeleteSelectedNodes();

   CurrentGraphEditor->ClearSelectionSet();

   for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
   {
      if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
      {
         CurrentGraphEditor->SetNodeSelection(Node, true);
      }
   }
}

void FOSEGenericGraphAssetEditor::CutSelectedNodes()
{
   CopySelectedNodes();
   DeleteSelectedDuplicatableNodes();
}

bool FOSEGenericGraphAssetEditor::CanCutNodes()
{
   return CanCopyNodes() && CanDeleteNodes();
}

void FOSEGenericGraphAssetEditor::CopySelectedNodes()
{
   // Export the selected nodes and place the text on the clipboard
   FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

   FString ExportedText;

   for (FGraphPanelSelectionSet::TIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
   {
      UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
      if (Node == nullptr)
      {
         SelectedIter.RemoveCurrent();
         continue;
      }

      if (UOSEGenericGraphEdNodeEdge* EdNode_Edge = Cast<UOSEGenericGraphEdNodeEdge>(*SelectedIter))
      {
         UOSEGenericGraphEdNode* StartNode = EdNode_Edge->GetStartNode();
         UOSEGenericGraphEdNode* EndNode = EdNode_Edge->GetEndNode();

         if (!SelectedNodes.Contains(StartNode) || !SelectedNodes.Contains(EndNode))
         {
            SelectedIter.RemoveCurrent();
            continue;
         }
      }

      Node->PrepareForCopying();
   }

   FEdGraphUtilities::ExportNodesToText(SelectedNodes, ExportedText);
   FPlatformApplicationMisc::ClipboardCopy(*ExportedText);
}

bool FOSEGenericGraphAssetEditor::CanCopyNodes()
{
   // If any of the nodes can be duplicated then we should allow copying
   const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
   for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
   {
      UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
      if (Node && Node->CanDuplicateNode())
      {
         return true;
      }
   }

   return false;
}

void FOSEGenericGraphAssetEditor::PasteNodes()
{
   TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
   if (CurrentGraphEditor.IsValid())
   {
      PasteNodesHere(CurrentGraphEditor->GetPasteLocation());
   }
}

void FOSEGenericGraphAssetEditor::PasteNodesHere(const FVector2D& Location)
{
   // Find the graph editor with focus
   TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
   if (!CurrentGraphEditor.IsValid())
   {
      return;
   }
   // Select the newly pasted stuff
   UEdGraph* EdGraph = CurrentGraphEditor->GetCurrentGraph();

   {
      const FScopedTransaction Transaction(FGenericCommands::Get().Paste->GetDescription());
      EdGraph->Modify();

      // Clear the selection set (newly pasted stuff will be selected)
      CurrentGraphEditor->ClearSelectionSet();

      // Grab the text to paste from the clipboard.
      FString TextToImport;
      FPlatformApplicationMisc::ClipboardPaste(TextToImport);

      // Import the nodes
      TSet<UEdGraphNode*> PastedNodes;
      FEdGraphUtilities::ImportNodesFromText(EdGraph, TextToImport, PastedNodes);

      //Average position of nodes so we can move them while still maintaining relative distances to each other
      FVector2D AvgNodePosition(0.0f, 0.0f);

      for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
      {
         UEdGraphNode* Node = *It;
         AvgNodePosition.X += Node->NodePosX;
         AvgNodePosition.Y += Node->NodePosY;
      }

      float InvNumNodes = 1.0f / float(PastedNodes.Num());
      AvgNodePosition.X *= InvNumNodes;
      AvgNodePosition.Y *= InvNumNodes;

      for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
      {
         UEdGraphNode* Node = *It;
         CurrentGraphEditor->SetNodeSelection(Node, true);

         Node->NodePosX = (Node->NodePosX - AvgNodePosition.X) + Location.X;
         Node->NodePosY = (Node->NodePosY - AvgNodePosition.Y) + Location.Y;

         Node->SnapToGrid(16);

         // Give new node a different Guid from the old one
         Node->CreateNewGuid();
      }
   }

   // Update UI
   CurrentGraphEditor->NotifyGraphChanged();

   UObject* GraphOwner = EdGraph->GetOuter();
   if (GraphOwner)
   {
      GraphOwner->PostEditChange();
      GraphOwner->MarkPackageDirty();
   }
}

bool FOSEGenericGraphAssetEditor::CanPasteNodes()
{
   TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
   if (!CurrentGraphEditor.IsValid())
   {
      return false;
   }

   FString ClipboardContent;
   FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

   return FEdGraphUtilities::CanImportNodesFromText(CurrentGraphEditor->GetCurrentGraph(), ClipboardContent);
}

void FOSEGenericGraphAssetEditor::DuplicateNodes()
{
   CopySelectedNodes();
   PasteNodes();
}

bool FOSEGenericGraphAssetEditor::CanDuplicateNodes()
{
   return CanCopyNodes();
}

void FOSEGenericGraphAssetEditor::GraphSettings()
{
   PropertyWidget->SetObject(EditingGraph);
}

bool FOSEGenericGraphAssetEditor::CanGraphSettings() const
{
   return true;
}

void FOSEGenericGraphAssetEditor::AutoArrange()
{
   UOSEGenericGraphEdGraph* EdGraph = Cast<UOSEGenericGraphEdGraph>(EditingGraph->EdGraph);
   check(EdGraph != nullptr);

   const FScopedTransaction Transaction(LOCTEXT("GenericGraphEditorAutoArrange", "Generic Graph Editor: Auto Arrange"));

   EdGraph->Modify();

   UOSEAutoLayoutStrategy* LayoutStrategy = nullptr;
   switch (GenericGraphEditorSettings.AutoLayoutStrategy)
   {
   case EOSEGenericGraphAutoLayoutStrategy::Tree:
      LayoutStrategy = NewObject<UOSEAutoLayoutStrategy>(EdGraph, UOSETreeLayoutStrategy::StaticClass());
      break;
   case EOSEGenericGraphAutoLayoutStrategy::ForceDirected:
      LayoutStrategy = NewObject<UOSEAutoLayoutStrategy>(EdGraph, UOSEForceDirectedLayoutStrategy::StaticClass());
      break;
   default:
      break;
   }

   if (LayoutStrategy != nullptr)
   {
      LayoutStrategy->Settings = &GenericGraphEditorSettings;
      LayoutStrategy->Layout(EdGraph);
      LayoutStrategy->ConditionalBeginDestroy();
   }
   else
   {
      LOG_ERROR(TEXT("FOSEGenericGraphAssetEditor::AutoArrange LayoutStrategy is null."));
   }
}

bool FOSEGenericGraphAssetEditor::CanAutoArrange() const
{
   return EditingGraph != nullptr && Cast<UOSEGenericGraphEdGraph>(EditingGraph->EdGraph) != nullptr;
}

void FOSEGenericGraphAssetEditor::RefreshGraph()
{
   if (TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor())
   {
      CurrentGraphEditor->NotifyGraphChanged();
      RebuildGenericGraph();
   }
}

bool FOSEGenericGraphAssetEditor::CanRefreshGraph() const
{
   return EditingGraph != nullptr && Cast<UOSEGenericGraphEdGraph>(EditingGraph->EdGraph) != nullptr;
}

void FOSEGenericGraphAssetEditor::OnRenameNode()
{
   TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
   if (CurrentGraphEditor.IsValid())
   {
      const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
      for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
      {
         UEdGraphNode* SelectedNode = Cast<UEdGraphNode>(*NodeIt);
         if (SelectedNode != NULL && SelectedNode->bCanRenameNode)
         {
            CurrentGraphEditor->IsNodeTitleVisible(SelectedNode, true);
            break;
         }
      }
   }
}

bool FOSEGenericGraphAssetEditor::CanRenameNodes() const
{
   UOSEGenericGraphEdGraph* EdGraph = Cast<UOSEGenericGraphEdGraph>(EditingGraph->EdGraph);
   check(EdGraph != nullptr);

   UOSEGenericGraph* Graph = EdGraph->GetGenericGraph();
   check(Graph != nullptr)

   return Graph->bCanRenameNode && GetSelectedNodes().Num() == 1;
}

bool FOSEGenericGraphAssetEditor::IsGraphPropertyVisible(const FPropertyAndParent& Prop) const
{
   const TArray<TWeakObjectPtr<UObject>>& PropertyWidgetObjects = PropertyWidget->GetSelectedObjects();
   const bool bIsGraphProperty = PropertyWidgetObjects.Num() == 1 && PropertyWidgetObjects[0].Get() == EditingGraph;
   //const bool bIsEdit = Prop.Property.HasAnyPropertyFlags(CPF_Edit);
   //const bool isPublicVariable = !Prop.Property.HasAnyPropertyFlags(CPF_DisableEditOnInstance);
   //const bool bIsEditInstanceOnly = Prop.Property.HasAnyPropertyFlags(CPF_DisableEditOnTemplate);
   const bool bIsEditDefaultsOnly = Prop.Property.HasAnyPropertyFlags(CPF_DisableEditOnInstance);
   if (bIsEditDefaultsOnly)
   {
      return bIsGraphProperty ? GetSettings().ShowAdvancedGraphProperties : false;
   }
   return true;
}

void FOSEGenericGraphAssetEditor::OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection)
{
   TArray<UObject*> Selection;

   for (UObject* SelectionEntry : NewSelection)
   {
      Selection.Add(SelectionEntry);
   }

   if (Selection.Num() == 0)
   {
      PropertyWidget->SetObject(EditingGraph);

   }
   else
   {
      PropertyWidget->SetObjects(Selection);
   }
}

void FOSEGenericGraphAssetEditor::OnNodeDoubleClicked(UEdGraphNode* Node)
{
   UOSEGenericGraphEdNode* GenericNode = Cast<UOSEGenericGraphEdNode>(Node);
   if (GenericNode != nullptr && GenericNode->GenericGraphNode != nullptr)
   {
      GenericNode->GenericGraphNode->OnNodeDoubleClicked();
   }
}

void FOSEGenericGraphAssetEditor::OnNodeTextCommitted(const FText& NewText, ETextCommit::Type CommitType, UEdGraphNode* Node)
{
   if (Node != nullptr)
   {
      Node->Modify();
      Node->OnRenameNode(NewText.ToString());
   }
}

FReply FOSEGenericGraphAssetEditor::OnSpawnNodeByShortcut(FInputChord Chord, const FVector2D& Location)
{
   if (EditingGraph == nullptr || EditingGraph->EdGraph == nullptr)
   {
      return FReply::Unhandled();
   }

   if (Chord.Key == EKeys::C)
   {
      if (TSharedPtr<FEdGraphSchemaAction> CreateCommentAction = EditingGraph->EdGraph->GetSchema()->GetCreateCommentAction())
      {
         constexpr bool bSelectNewNode = true;
         CreateCommentAction->PerformAction(EditingGraph->EdGraph, nullptr, Location, bSelectNewNode);
         return FReply::Handled();
      }
   }

   return FReply::Unhandled();
}

void FOSEGenericGraphAssetEditor::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
   if (EditingGraph == nullptr)
      return;

   EditingGraph->EdGraph->GetSchema()->ForceVisualizationCacheClear();
}

#if ENGINE_MAJOR_VERSION < 5
void FOSEGenericGraphAssetEditor::OnPackageSaved(const FString& PackageFileName, UObject* Outer)
{
   RebuildGenericGraph();
}
#else // #if ENGINE_MAJOR_VERSION < 5
void FOSEGenericGraphAssetEditor::OnPackageSavedWithContext(const FString& PackageFileName, UPackage* Package, FObjectPostSaveContext ObjectSaveContext)
{
   RebuildGenericGraph();
}
#endif // #else // #if ENGINE_MAJOR_VERSION < 5

void FOSEGenericGraphAssetEditor::RegisterToolbarTab(const TSharedRef<class FTabManager>& InTabManager)
{
   FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
}


#undef LOCTEXT_NAMESPACE

