// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "VoiceOver/OSEVoiceOverConversationGraphEditor.h"

//ose
#include "VoiceOver/OSEVoiceOverConversation.h"

//ose editor
#include "VoiceOver/OSEVoiceOverConversationGraph.h"
#include "VoiceOver/OSEVoiceOverConversationGraphNode.h"
#include "VoiceOver/OSEVoiceOverConversationGraphSchema.h"


//ue4
#include "EdGraph/EdGraph.h"
#include "Editor.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Docking/TabManager.h"
#include "GraphEditor.h"
#include "GraphEditAction.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "OSEVoiceOverConversationGraphEditor"

DEFINE_LOG_CATEGORY_STATIC(LogOSEConversationEditor, Log, All);

const FName FOSEVoiceOverConversationGraphEditor::GraphCanvasTabId(TEXT("OSEConversationEditor_GraphCanvas"));
const FName FOSEVoiceOverConversationGraphEditor::PropertiesTabId(TEXT("OSEConversationEditor_Properties"));

// While we don't need this graph editor subclass yet, it might be useful in the future so we'll stub it in here.
class SOSEVoiceOverConversationGraphEditor : public SGraphEditor
{
private:
   TWeakPtr<FOSEVoiceOverConversationGraphEditor> _conversationGraphEditor;

public:
   SOSEVoiceOverConversationGraphEditor()
      : SGraphEditor()
      , _conversationGraphEditor(nullptr)
   {
   }

   void Construct(const FArguments& args, TSharedPtr<FOSEVoiceOverConversationGraphEditor> editor)
   {
      _conversationGraphEditor = editor;
      SGraphEditor::Construct(args);
   }
};


void FOSEVoiceOverConversationGraphEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
   WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_ConversationEditor", "Conversation Editor"));
   auto workspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

   FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

   InTabManager->RegisterTabSpawner(GraphCanvasTabId, FOnSpawnTab::CreateSP(this, &FOSEVoiceOverConversationGraphEditor::SpawnTab_GraphCanvas))
      .SetDisplayName(LOCTEXT("GraphCanvasTab", "Graph"))
      .SetGroup(workspaceMenuCategoryRef)
      .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x"));

   InTabManager->RegisterTabSpawner(PropertiesTabId, FOnSpawnTab::CreateSP(this, &FOSEVoiceOverConversationGraphEditor::SpawnTab_Properties))
      .SetDisplayName(LOCTEXT("PropertiesTab", "Details"))
      .SetGroup(workspaceMenuCategoryRef)
      .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FOSEVoiceOverConversationGraphEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
   FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

   InTabManager->UnregisterTabSpawner(GraphCanvasTabId);
   InTabManager->UnregisterTabSpawner(PropertiesTabId);
}

void FOSEVoiceOverConversationGraphEditor::Init(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit)
{
   UOSEVoiceOverConversation* conversation = CastChecked<UOSEVoiceOverConversation>(ObjectToEdit);

   GEditor->RegisterForUndo(this);

   ToolkitCommands->MapAction
   (
      FGenericCommands::Get().Undo,
      FExecuteAction::CreateSP(this, &FOSEVoiceOverConversationGraphEditor::UndoGraphAction)
   );

   ToolkitCommands->MapAction
   (
      FGenericCommands::Get().Redo,
      FExecuteAction::CreateSP(this, &FOSEVoiceOverConversationGraphEditor::RedoGraphAction)
   );


   if (!IsValid(conversation->EdGraph))
   {
      UOSEVoiceOverConversationGraph* conversationGraph = CastChecked<UOSEVoiceOverConversationGraph>(FBlueprintEditorUtils::CreateNewGraph(conversation, NAME_None, UOSEVoiceOverConversationGraph::StaticClass(), UOSEVoiceOverConversationGraphSchema::StaticClass()));
      conversation->EdGraph = conversationGraph;
   }

   CreateInternalWidgets(conversation);

   TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_ConversationGraphEditor_Layout_v4")
      ->AddArea
      (
         FTabManager::NewPrimaryArea()
         ->SetOrientation(Orient_Vertical)
         ->Split
         (
            FTabManager::NewSplitter()
            ->Split
            (
               FTabManager::NewStack()
               ->SetHideTabWell(true)
               ->SetSizeCoefficient(0.2f)
               ->AddTab(PropertiesTabId, ETabState::OpenedTab)
            )
            ->Split
            (
               FTabManager::NewStack()
               ->SetHideTabWell(true)
               ->SetSizeCoefficient(0.8f)
               ->AddTab(GraphCanvasTabId, ETabState::OpenedTab)
            )
         )
      );

   const bool bCreateDefaultStandaloneMenu = true;
   const bool bCreateDefaultToolbar = true;
   FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, TEXT("OSEConversationEditorApp"), StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, conversation);
}

FOSEVoiceOverConversationGraphEditor::~FOSEVoiceOverConversationGraphEditor()
{
   GEditor->UnregisterForUndo(this);
   DetailsView.Reset();
}


TSharedRef<SDockTab> FOSEVoiceOverConversationGraphEditor::SpawnTab_GraphCanvas(const FSpawnTabArgs& Args)
{
   check(Args.GetTabId() == GraphCanvasTabId);

   TSharedRef<SDockTab> spawnedTab = SNew(SDockTab)
      .Label(LOCTEXT("GraphCanvasTitle", "Graph"))
      [
         GraphEditor.ToSharedRef()
      ];

   return spawnedTab;
}

TSharedRef<SDockTab> FOSEVoiceOverConversationGraphEditor::SpawnTab_Properties(const FSpawnTabArgs& Args)
{
   check(Args.GetTabId() == PropertiesTabId);

   TSharedRef<SDockTab> spawnedTab = SNew(SDockTab)
      .Label(LOCTEXT("OSEVoiceOverConversationPropertiesTitle", "Details"))
      [
         DetailsView.ToSharedRef()
      ];
   spawnedTab->SetTabIcon(FAppStyle::GetBrush("LevelEditor.Tabs.Details"));

   return spawnedTab;
}

FName FOSEVoiceOverConversationGraphEditor::GetToolkitFName() const
{
   return FName("OSEVoiceOverConversationEditor");
}

FText FOSEVoiceOverConversationGraphEditor::GetBaseToolkitName() const
{
   return LOCTEXT("AppLabel", "Conversation Editor");
}

FText FOSEVoiceOverConversationGraphEditor::GetToolkitToolTipText() const
{
   return GetToolTipTextForObject(GetEditingObjects()[0]);
}

FString FOSEVoiceOverConversationGraphEditor::GetWorldCentricTabPrefix() const
{
   return LOCTEXT("WorldCentricTabPrefix", "Conversation ").ToString();
}

FLinearColor FOSEVoiceOverConversationGraphEditor::GetWorldCentricTabColorScale() const
{
   return FLinearColor(0.2f, 0.4f, 0.8f, 0.5f);
}

void FOSEVoiceOverConversationGraphEditor::CreateInternalWidgets(UOSEVoiceOverConversation* conversation)
{
   GraphEditor = CreateGraphEditorWidget(conversation);
   FPropertyEditorModule& propertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
   FDetailsViewArgs args;
   args.bUpdatesFromSelection = false;
   args.bLockable = false;
   args.bAllowSearch = true;
   args.NameAreaSettings = FDetailsViewArgs::HideNameArea;
   args.bHideSelectionTip = false;
   DetailsView = propertyEditorModule.CreateDetailView(args);
   DetailsView->SetObject(conversation);
}

TSharedRef<SGraphEditor> FOSEVoiceOverConversationGraphEditor::CreateGraphEditorWidget(UOSEVoiceOverConversation* conversation)
{
   _SetupGraphCommands();

   FGraphAppearanceInfo appearanceInfo;
   appearanceInfo.CornerText = LOCTEXT("AppearanceCornerText_OSEVoiceOverConversation", "CONVERSATION");

   SGraphEditor::FGraphEditorEvents events;
   events.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FOSEVoiceOverConversationGraphEditor::OnSelectedNodesChanged);

   return SNew(SOSEVoiceOverConversationGraphEditor, SharedThis(this))
      .AdditionalCommands(GraphEditorCommands)
      .IsEditable(true)
      .Appearance(appearanceInfo)
      .GraphToEdit(conversation->EdGraph)
      .GraphEvents(events)
      .ShowGraphStateOverlay(false);
}

void FOSEVoiceOverConversationGraphEditor::OnSelectedNodesChanged(const TSet<UObject*>& NewSelection)
{
   if (NewSelection.Num() > 0)
   {
      TArray<UObject*> selection;
      for (UObject* obj : NewSelection)
      {
         UOSEVoiceOverConversationGraphNode* graphNode = CastChecked<UOSEVoiceOverConversationGraphNode>(obj);
         selection.Add(graphNode->ConversationNode);
      }
      DetailsView->SetObjects(selection);
   }
   else
   {
      DetailsView->SetObject(GetEditingObjects()[0]);
   }
}

// Graph Command Handlers

bool FOSEVoiceOverConversationGraphEditor::_CanSelectAllNodes() const
{
   return true;
}

void FOSEVoiceOverConversationGraphEditor::_SelectAllNodes()
{
   GraphEditor->SelectAllNodes();
}

void FOSEVoiceOverConversationGraphEditor::_RemoveSelectedNodes()
{
   if (!GraphEditor.IsValid())
   {
      return;
   }

   const FScopedTransaction Transaction(FGenericCommands::Get().Delete->GetDescription());
   FGraphPanelSelectionSet selection = GraphEditor->GetSelectedNodes();
   for (FGraphPanelSelectionSet::TConstIterator it(selection); it; ++it)
   {
      UOSEVoiceOverConversationGraphNode* node = Cast<UOSEVoiceOverConversationGraphNode>(*it);

      if (!IsValid(node))
      {
         continue;
      }

      _RemoveNode(*node);
   }
}

bool FOSEVoiceOverConversationGraphEditor::_CanRemoveNodes() const
{
   return GraphEditor->GetSelectedNodes().Num() > 0;
}

void FOSEVoiceOverConversationGraphEditor::_CutSelectedNodes()
{
   _CopySelectedNodes();
   _RemoveSelectedNodes();
}

bool FOSEVoiceOverConversationGraphEditor::_CanCutNodes() const
{
   return _CanCopyNodes() && _CanRemoveNodes();
}

void FOSEVoiceOverConversationGraphEditor::_CopySelectedNodes()
{
   FGraphPanelSelectionSet selection = GraphEditor->GetSelectedNodes();

   for (FGraphPanelSelectionSet::TIterator it(selection); it; ++it)
   {
      UEdGraphNode* node = Cast<UEdGraphNode>(*it);
      if (node == nullptr)
      {
         it.RemoveCurrent();
         continue;
      }

      node->PrepareForCopying();
   }

   FString exportedText;
   FEdGraphUtilities::ExportNodesToText(selection, exportedText);
   FPlatformApplicationMisc::ClipboardCopy(*exportedText);
}

bool FOSEVoiceOverConversationGraphEditor::_CanCopyNodes() const
{
   // If any of the nodes can be duplicated then we should allow copying
   const FGraphPanelSelectionSet selected = GraphEditor->GetSelectedNodes();
   for (FGraphPanelSelectionSet::TConstIterator it(selected); it; ++it)
   {
      UEdGraphNode* node = Cast<UEdGraphNode>(*it);
      if (node && node->CanDuplicateNode())
      {
         return true;
      }
   }
   return false;
}

void FOSEVoiceOverConversationGraphEditor::_PasteNodes()
{
   _PasteNodesHere(GraphEditor->GetPasteLocation());
}

void FOSEVoiceOverConversationGraphEditor::_PasteNodesHere(const FVector2D& location)
{
   FString importText;
   FPlatformApplicationMisc::ClipboardPaste(importText);

   UEdGraph* graph = GraphEditor->GetCurrentGraph();

   UOSEVoiceOverConversation* conversation = CastChecked<UOSEVoiceOverConversation>(graph->GetOuter());

   // Import the nodes
   TSet<UEdGraphNode*> pastedNodes;
   FEdGraphUtilities::ImportNodesFromText(graph, importText, pastedNodes);

   //Adjust the nodes around the average position.
   int32 avgCount = 0;
   FVector2D avgNodePosition(0.0f, 0.0f);

   for (TSet<UEdGraphNode*>::TIterator it(pastedNodes); it; ++it)
   {
      UOSEVoiceOverConversationGraphNode* node = CastChecked<UOSEVoiceOverConversationGraphNode>(*it);
      // Manually duplicate the conversation node itself since when it's exported to text a pointer is used.
      // Make sure to set it's outer to the conversation.
      UOSEVoiceOverConversationNode* nodeCopy = DuplicateObject(node->ConversationNode, conversation);

      //Also manually fix up the conversation pointer.
      node->Conversation = conversation;
      node->ConversationNode = nodeCopy;
      nodeCopy->Conversation = conversation;
   }

   //Calculate the average position of the nodes to offset them around the paste location.
   for (TSet<UEdGraphNode*>::TIterator it(pastedNodes); it; ++it)
   {
      UEdGraphNode* node = *it;
      avgNodePosition.X += node->NodePosX;
      avgNodePosition.Y += node->NodePosY;
      ++avgCount;
   }

   if (avgCount > 0)
   {
      float invNumNodes = 1.0f / float(avgCount);
      avgNodePosition.X *= invNumNodes;
      avgNodePosition.Y *= invNumNodes;
   }

   GraphEditor->ClearSelectionSet();
   for (TSet<UEdGraphNode*>::TIterator it(pastedNodes); it; ++it)
   {
      UEdGraphNode* node = *it;
      // Select the newly pasted stuff
      GraphEditor->SetNodeSelection(node, true);

      //Adjust their position.
      node->NodePosX = (node->NodePosX - avgNodePosition.X) + location.X;
      node->NodePosY = (node->NodePosY - avgNodePosition.Y) + location.Y;

      node->SnapToGrid(16);
      node->CreateNewGuid();
   }

   GraphEditor->NotifyGraphChanged();

   //Mark the asset dirty.
   UObject* graphOwner = graph->GetOuter();
   if (graphOwner)
   {
      graphOwner->PostEditChange();
      graphOwner->MarkPackageDirty();
   }
}

bool FOSEVoiceOverConversationGraphEditor::_CanPasteNodes() const
{
   if (!GraphEditor.IsValid())
   {
      return false;
   }

   FString clipboardContent;
   FPlatformApplicationMisc::ClipboardPaste(clipboardContent);

   return FEdGraphUtilities::CanImportNodesFromText(GraphEditor->GetCurrentGraph(), clipboardContent);
}

void FOSEVoiceOverConversationGraphEditor::_DuplicateNodes()
{
   _CopySelectedNodes();
   _PasteNodes();
}

bool FOSEVoiceOverConversationGraphEditor::_CanDuplicateNodes() const
{
   return _CanCopyNodes();
}

void FOSEVoiceOverConversationGraphEditor::UndoGraphAction()
{
   GEditor->UndoTransaction();
}

void FOSEVoiceOverConversationGraphEditor::RedoGraphAction()
{
   // Clear selection, to avoid holding refs to nodes that go away
   GraphEditor->ClearSelectionSet();

   GEditor->RedoTransaction();
}

void FOSEVoiceOverConversationGraphEditor::_SetupGraphCommands()
{
   if (GraphEditorCommands.IsValid())
   {
      return;
   }

   GraphEditorCommands = MakeShareable(new FUICommandList);

   // Can't use CreateSP here because derived editor are already implementing TSharedFromThis<FAssetEditorToolkit>
   // however it should be safe, since commands are being used only within this editor
   // if it ever crashes, this function will have to go away and be reimplemented in each derived class

   GraphEditorCommands->MapAction(FGenericCommands::Get().SelectAll,
      FExecuteAction::CreateRaw(this, &FOSEVoiceOverConversationGraphEditor::_SelectAllNodes),
      FCanExecuteAction::CreateRaw(this, &FOSEVoiceOverConversationGraphEditor::_CanSelectAllNodes)
   );

   GraphEditorCommands->MapAction(FGenericCommands::Get().Delete,
      FExecuteAction::CreateRaw(this, &FOSEVoiceOverConversationGraphEditor::_RemoveSelectedNodes),
      FCanExecuteAction::CreateRaw(this, &FOSEVoiceOverConversationGraphEditor::_CanRemoveNodes)
   );

   GraphEditorCommands->MapAction(FGenericCommands::Get().Copy,
      FExecuteAction::CreateRaw(this, &FOSEVoiceOverConversationGraphEditor::_CopySelectedNodes),
      FCanExecuteAction::CreateRaw(this, &FOSEVoiceOverConversationGraphEditor::_CanCopyNodes)
   );

   GraphEditorCommands->MapAction(FGenericCommands::Get().Cut,
      FExecuteAction::CreateRaw(this, &FOSEVoiceOverConversationGraphEditor::_CutSelectedNodes),
      FCanExecuteAction::CreateRaw(this, &FOSEVoiceOverConversationGraphEditor::_CanCutNodes)
   );

   GraphEditorCommands->MapAction(FGenericCommands::Get().Paste,
      FExecuteAction::CreateRaw(this, &FOSEVoiceOverConversationGraphEditor::_PasteNodes),
      FCanExecuteAction::CreateRaw(this, &FOSEVoiceOverConversationGraphEditor::_CanPasteNodes)
   );

   GraphEditorCommands->MapAction(FGenericCommands::Get().Duplicate,
      FExecuteAction::CreateRaw(this, &FOSEVoiceOverConversationGraphEditor::_DuplicateNodes),
      FCanExecuteAction::CreateRaw(this, &FOSEVoiceOverConversationGraphEditor::_CanDuplicateNodes)
   );
}

void FOSEVoiceOverConversationGraphEditor::_RemoveNode(UOSEVoiceOverConversationGraphNode& node)
{
   if (!node.GetSchema() || !node.GetGraph())
      return;

   node.GetSchema()->SafeDeleteNodeFromGraph(node.GetGraph(), &node);
}


void FOSEVoiceOverConversationGraphEditor::AddReferencedObjects(FReferenceCollector& collector)
{
   for (TObjectPtr<UObject> obj : GetEditingObjects())
   {
      collector.AddReferencedObject(obj);
   }
}


UEdGraph* FOSEVoiceOverConversationGraphEditor::GetGraph()
{
   return GraphEditor->GetCurrentGraph();
}

FText FOSEVoiceOverConversationGraphEditor::GetToolkitName() const
{
   UObject* EditObject = GetEditingObjects()[0];
   return GetLabelForObject(EditObject);
}

void FOSEVoiceOverConversationGraphEditor::PostUndo(bool bSuccess)
{
   GraphEditor->ClearSelectionSet();
   GraphEditor->NotifyGraphChanged();
}


#undef LOCTEXT_NAMESPACE
