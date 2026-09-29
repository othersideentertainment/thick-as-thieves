// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#include "GenericGraphAssetEditor/OSEGenericGraphAssetSchema.h"
#include "ToolMenus.h"
#include "OSEGenericGraphEditorPCH.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEdNode.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEdNodeEdge.h"
#include "GenericGraphAssetEditor/OSEGenericGraphConnectionDrawingPolicy.h"
#include "GraphEditorActions.h"
#include "ScopedTransaction.h"
#include "Framework/Commands/GenericCommands.h"
#include "AutoLayout/OSEForceDirectedLayoutStrategy.h"
#include "AutoLayout/OSETreeLayoutStrategy.h"

#define LOCTEXT_NAMESPACE "AssetSchema_GenericGraph"

int32 UOSEGenericGraphAssetSchema::CurrentCacheRefreshID = 0;


class FNodeVisitorCycleChecker
{
public:
   /** Check whether a loop in the graph would be caused by linking the passed-in nodes */
   bool CheckForLoop(UEdGraphNode* StartNode, UEdGraphNode* EndNode)
   {

      VisitedNodes.Add(StartNode);

      return TraverseNodes(EndNode);
   }

private:
   bool TraverseNodes(UEdGraphNode* Node)
   {
      VisitedNodes.Add(Node);

      for (auto MyPin : Node->Pins)
      {
         if (MyPin->Direction == EGPD_Output)
         {
            for (auto OtherPin : MyPin->LinkedTo)
            {
               UEdGraphNode* OtherNode = OtherPin->GetOwningNode();
               if (VisitedNodes.Contains(OtherNode))
               {
                  // Only  an issue if this is a back-edge
                  return false;
               }
               else if (!FinishedNodes.Contains(OtherNode))
               {
                  // Only should traverse if this node hasn't been traversed
                  if (!TraverseNodes(OtherNode))
                     return false;
               }
            }
         }
      }

      VisitedNodes.Remove(Node);
      FinishedNodes.Add(Node);
      return true;
   };


   TSet<UEdGraphNode*> VisitedNodes;
   TSet<UEdGraphNode*> FinishedNodes;
};

UEdGraphNode* FOSEGenericGraphAssetSchemaAction_NewNode::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode /*= true*/)
{
   UEdGraphNode* ResultNode = nullptr;

   if (NodeTemplate != nullptr)
   {
      const FScopedTransaction Transaction(LOCTEXT("GenericGraphEditorNewNode", "Generic Graph Editor: New Node"));
      ParentGraph->Modify();
      if (FromPin != nullptr)
         FromPin->Modify();

      NodeTemplate->Rename(nullptr, ParentGraph);
      ParentGraph->AddNode(NodeTemplate, true, bSelectNewNode);

      NodeTemplate->CreateNewGuid();
      NodeTemplate->PostPlacedNewNode();
      NodeTemplate->AllocateDefaultPins();
      NodeTemplate->AutowireNewNode(FromPin);

      NodeTemplate->NodePosX = Location.X;
      NodeTemplate->NodePosY = Location.Y;

      NodeTemplate->GenericGraphNode->SetFlags(RF_Transactional);
      NodeTemplate->SetFlags(RF_Transactional);

      ResultNode = NodeTemplate;
   }

   return ResultNode;
}

void FOSEGenericGraphAssetSchemaAction_NewNode::AddReferencedObjects(FReferenceCollector& Collector)
{
   FEdGraphSchemaAction::AddReferencedObjects(Collector);
   Collector.AddReferencedObject(NodeTemplate);
}

UEdGraphNode* FOSEGenericGraphAssetSchemaAction_NewEdge::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode /*= true*/)
{
   UEdGraphNode* ResultNode = nullptr;

   if (NodeTemplate != nullptr)
   {
      const FScopedTransaction Transaction(LOCTEXT("GenericGraphEditorNewEdge", "Generic Graph Editor: New Edge"));
      ParentGraph->Modify();
      if (FromPin != nullptr)
         FromPin->Modify();

      NodeTemplate->Rename(nullptr, ParentGraph);
      ParentGraph->AddNode(NodeTemplate, true, bSelectNewNode);

      NodeTemplate->CreateNewGuid();
      NodeTemplate->PostPlacedNewNode();
      NodeTemplate->AllocateDefaultPins();
      NodeTemplate->AutowireNewNode(FromPin);

      NodeTemplate->NodePosX = Location.X;
      NodeTemplate->NodePosY = Location.Y;

      NodeTemplate->GenericGraphEdge->SetFlags(RF_Transactional);
      NodeTemplate->SetFlags(RF_Transactional);

      ResultNode = NodeTemplate;
   }

   return ResultNode;
}

void FOSEGenericGraphAssetSchemaAction_NewEdge::AddReferencedObjects(FReferenceCollector& Collector)
{
   FEdGraphSchemaAction::AddReferencedObjects(Collector);
   Collector.AddReferencedObject(NodeTemplate);
}

UEdGraphNode* FOSEGenericGraphAssetSchemaAction_NewComment::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode /*= true*/)
{
   // Add menu item for creating comment boxes
   UEdGraphNode_Comment* CommentTemplate = NewObject<UEdGraphNode_Comment>();

   FVector2D SpawnLocation = Location;
   FSlateRect Bounds;

   TSharedPtr<SGraphEditor> GraphEditorPtr = SGraphEditor::FindGraphEditorForGraph(ParentGraph);
   if (GraphEditorPtr && GraphEditorPtr->GetBoundsForSelectedNodes(Bounds, 50.0f))
   {
      CommentTemplate->SetBounds(Bounds);
      SpawnLocation.X = CommentTemplate->NodePosX;
      SpawnLocation.Y = CommentTemplate->NodePosY;
   }

   CommentTemplate->bCommentBubbleVisible_InDetailsPanel = false;
   CommentTemplate->bCommentBubbleVisible = false;
   CommentTemplate->bCommentBubblePinned = false;

   UEdGraphNode* NewNode = FEdGraphSchemaAction_NewNode::SpawnNodeFromTemplate<UEdGraphNode_Comment>(ParentGraph, CommentTemplate, SpawnLocation, bSelectNewNode);

   return NewNode;
}

namespace GenericGraphHelpers
{
   FText FormatNodeConnection(UEdGraphPin* Pin)
   {
      check(Pin != nullptr);

      UOSEGenericGraphEdNode* ConnectedNode = nullptr;
      UOSEGenericGraphEdNodeEdge* ViaEdge = nullptr;
      if (UOSEGenericGraphEdNodeEdge* Edge = Cast<UOSEGenericGraphEdNodeEdge>(Pin->GetOwningNode()))
      {
         ViaEdge = Edge;
         ConnectedNode = Edge->GetEndNode();
      }

      const FText PinName = FText::FromName(Pin->PinName);

      FText EdgeName = FText::GetEmpty();
      if (ViaEdge != nullptr)
      {
         EdgeName = ViaEdge->GetNodeTitle(ENodeTitleType::ListView);
         if (EdgeName.IsEmptyOrWhitespace())
         {
            EdgeName = FText::FromString(TEXT("Edge"));
         }
      }

      FText NodeName = (ConnectedNode != nullptr)
         ? ConnectedNode->GetNodeTitle(ENodeTitleType::ListView)
         : Pin->GetOwningNode()->GetNodeTitle(ENodeTitleType::ListView);
      if (NodeName.IsEmptyOrWhitespace())
      {
         NodeName = FText::FromString(TEXT("Node"));
      }

      FFormatNamedArguments Args;
      Args.Add(TEXT("Pin"), PinName);
      Args.Add(TEXT("Node"), NodeName);
      Args.Add(TEXT("Edge"), EdgeName);

      if (!PinName.IsEmpty() && !EdgeName.IsEmpty())
      {
         return FText::Format(LOCTEXT("NodeConnection_PinAndEdge", "{Node} (via {Edge}, {Pin})"), Args);
      }
      if (PinName.IsEmpty())
      {
         return FText::Format(LOCTEXT("NodeConnection_Edge", "{Node} (via {Edge})"), Args);
      }
      if (EdgeName.IsEmpty())
      {
         return FText::Format(LOCTEXT("NodeConnection_Edge", "{Node} ({Pin})"), Args);
      }
      return NodeName;
   }
}

void UOSEGenericGraphAssetSchema::GetBreakLinkToSubMenuActions(UToolMenu* Menu, UEdGraphPin* InGraphPin)
{
   // Make sure we have a unique name for every entry in the list
   TMap< uintptr_t, uint32 > LinkTitleCount;

   FToolMenuSection& Section = Menu->FindOrAddSection("GenericGraphAssetGraphSchemaPinActions");

   // Add all the links we could break from
   for (TArray<class UEdGraphPin*>::TConstIterator Links(InGraphPin->LinkedTo); Links; ++Links)
   {
      UEdGraphPin* Pin = *Links;

      uint32& Count = LinkTitleCount.FindOrAdd(reinterpret_cast<uintptr_t>(Pin->GetOwningNode()));

      FText Description;
      FFormatNamedArguments Args;
      Args.Add(TEXT("NodeTitle"), GenericGraphHelpers::FormatNodeConnection(Pin));
      Args.Add(TEXT("NumberOfNodes"), Count);

      if (Count == 0)
      {
         Description = FText::Format(LOCTEXT("BreakDesc", "Break link to {NodeTitle}"), Args);
      }
      else
      {
         Description = FText::Format(LOCTEXT("BreakDescMulti", "Break link to {NodeTitle} ({NumberOfNodes})"), Args);
      }

      ++Count;

      Section.AddMenuEntry(NAME_None, Description, Description, FSlateIcon(), FUIAction(
         FExecuteAction::CreateUObject(this, &UOSEGenericGraphAssetSchema::BreakSinglePinLink, const_cast<UEdGraphPin*>(InGraphPin), *Links)));
   }
}

EGraphType UOSEGenericGraphAssetSchema::GetGraphType(const UEdGraph* TestEdGraph) const
{
   return GT_StateMachine;
}

void UOSEGenericGraphAssetSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
   UOSEGenericGraph* Graph = CastChecked<UOSEGenericGraph>(ContextMenuBuilder.CurrentGraph->GetOuter());

   ContextMenuBuilder.AddAction(GetCreateCommentAction());

   if (Graph->NodeTypes.Num() == 0)
   {
      return;
   }

   const bool bNoParent = (ContextMenuBuilder.FromPin == nullptr);
   static const FText Category = LOCTEXT("GenericGraphNodeCategory", "Graph Node Types");
   static const FText AddToolTip = LOCTEXT("NewGenericGraphNodeTooltip", "Add node here");

   TSet<TSubclassOf<UOSEGenericGraphNode>, DefaultKeyFuncs<TSubclassOf<UOSEGenericGraphNode>>, TInlineSetAllocator<64>> Visited;

   // We need to be able to de-dup custom actions.
   // When you subclass a node type and include both types in a graph, the subclass will provide the same actions for both nodes and you'll get two of each.
   TSet<FOSEGenericGraphNodeCustomAction, DefaultKeyFuncs<FOSEGenericGraphNodeCustomAction>, TInlineSetAllocator<64>> AllCustomActions;

   auto AddNodeTypeToContextMenu = [Graph, &ContextMenuBuilder, &Visited, &AllCustomActions](TSubclassOf<UOSEGenericGraphNode> NodeType)
   {
      if (!NodeType
         || Visited.Contains(NodeType)
         || NodeType->HasAnyClassFlags(CLASS_Abstract)
         || !Graph->GetClass()->IsChildOf(NodeType.GetDefaultObject()->CompatibleGraphType)
         || NodeType->GetName().StartsWith("REINST")
         || NodeType->GetName().StartsWith("SKEL"))
      {
         return;
      }

      // Make sure the node type is a child class of any of the valid types for this graph
      bool IsChildOfGraphNodeType = false;
      for (const TSubclassOf<UOSEGenericGraphNode>& BaseNodeType : Graph->NodeTypes)
      {
         if (NodeType->IsChildOf(BaseNodeType))
         {
            IsChildOfGraphNodeType = true;
            break;
         }
      }
      if (!IsChildOfGraphNodeType)
      {
         return;
      }

      FText Desc = NodeType.GetDefaultObject()->ContextMenuName;
      if (Desc.IsEmpty())
      {
         FString Title = NodeType->GetName();
         Title.RemoveFromEnd("_C");
         Desc = FText::FromString(Title);
      }

      TSharedPtr<FOSEGenericGraphAssetSchemaAction_NewNode> NewNodeAction(new FOSEGenericGraphAssetSchemaAction_NewNode(Category, Desc, AddToolTip, 0));
      NewNodeAction->NodeTemplate = NewObject<UOSEGenericGraphEdNode>(ContextMenuBuilder.OwnerOfTemporaries);
      NewNodeAction->NodeTemplate->GenericGraphNode = NewObject<UOSEGenericGraphNode>(NewNodeAction->NodeTemplate, NodeType);
      NewNodeAction->NodeTemplate->GenericGraphNode->Graph = Graph;
      ContextMenuBuilder.AddAction(NewNodeAction);

      Visited.Add(NodeType);

      // Add any custom actions the node may provide
      TArray<FOSEGenericGraphNodeCustomAction> CustomActions;
      NodeType.GetDefaultObject()->GetCustomGraphContextMenuActions(CustomActions);
      for (const FOSEGenericGraphNodeCustomAction& CustomAction : CustomActions)
      {
         if (AllCustomActions.Contains(CustomAction))
         {
            continue;
         }
         TSharedPtr<FOSEGenericGraphAssetSchemaAction_NewNode> NewCustomNodeAction(new FOSEGenericGraphAssetSchemaAction_NewNode(CustomAction.Category, CustomAction.Label, CustomAction.TooltipText, 0));
         NewCustomNodeAction->NodeTemplate = NewObject<UOSEGenericGraphEdNode>(ContextMenuBuilder.OwnerOfTemporaries);
         NewCustomNodeAction->NodeTemplate->GenericGraphNode = CustomAction.Create(NewCustomNodeAction->NodeTemplate);
         if (ensure(NewCustomNodeAction->NodeTemplate->GenericGraphNode != nullptr))
         {
            NewCustomNodeAction->NodeTemplate->GenericGraphNode->Graph = Graph;
            ContextMenuBuilder.AddAction(NewCustomNodeAction);
            AllCustomActions.Add(CustomAction);
         }
      }
   };

   // Add the node types manually specified in the graph class
   for (TSubclassOf<UOSEGenericGraphNode> NodeType : Graph->NodeTypes)
   {
      AddNodeTypeToContextMenu(NodeType);
   }

   // Add any node types that are child classes of the node types specified in the graph class
   for (TObjectIterator<UClass> It; It; ++It)
   {
      TSubclassOf<UOSEGenericGraphNode> GenericNodeType = *It;
      if (!GenericNodeType)
      {
         continue;
      }
      for (TSubclassOf<UOSEGenericGraphNode> NodeType : Graph->NodeTypes)
      {
         if (GenericNodeType != NodeType && GenericNodeType->IsChildOf(NodeType))
         {
            AddNodeTypeToContextMenu(GenericNodeType);
         }
      }
   }
}

void UOSEGenericGraphAssetSchema::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
   if (Context->Pin)
   {
      {
         FToolMenuSection& Section = Menu->AddSection("GenericGraphAssetGraphSchemaNodeActions", LOCTEXT("PinActionsMenuHeader", "Pin Actions"));
         // Only display the 'Break Links' option if there is a link to break!
         if (Context->Pin->LinkedTo.Num() > 0)
         {
            Section.AddMenuEntry(FGraphEditorCommands::Get().BreakPinLinks);

            // add sub menu for break link to
            if (Context->Pin->LinkedTo.Num() > 1)
            {
               Section.AddSubMenu(
                  "BreakLinkTo",
                  LOCTEXT("BreakLinkTo", "Break Link To..."),
                  LOCTEXT("BreakSpecificLinks", "Break a specific link..."),
                  FNewToolMenuDelegate::CreateUObject((UOSEGenericGraphAssetSchema* const)this, &UOSEGenericGraphAssetSchema::GetBreakLinkToSubMenuActions, const_cast<UEdGraphPin*>(Context->Pin)));
            }
            else
            {
               ((UOSEGenericGraphAssetSchema* const)this)->GetBreakLinkToSubMenuActions(Menu, const_cast<UEdGraphPin*>(Context->Pin));
            }
         }
      }
   }
   else if (Context->Node)
   {
      {
         FToolMenuSection& Section = Menu->AddSection("GenericGraphAssetGraphSchemaNodeActions", LOCTEXT("ClassActionsMenuHeader", "Node Actions"));
         Section.AddMenuEntry(FGenericCommands::Get().Delete);
         Section.AddMenuEntry(FGenericCommands::Get().Cut);
         Section.AddMenuEntry(FGenericCommands::Get().Copy);
         Section.AddMenuEntry(FGenericCommands::Get().Duplicate);

         Section.AddMenuEntry(FGraphEditorCommands::Get().BreakNodeLinks);
      }
   }

   Super::GetContextMenuActions(Menu, Context);
}

TSharedPtr<FEdGraphSchemaAction> UOSEGenericGraphAssetSchema::GetCreateCommentAction() const
{
   return MakeShared<FOSEGenericGraphAssetSchemaAction_NewComment>(
      LOCTEXT("GenericGraphCommentNodeCategory", "Meta"),
      LOCTEXT("GenericGraphCommentNodeAction", "Comment"),
      LOCTEXT("NewGenericGraphCommentTooltip", "Add comment"),
      0);
}

const FPinConnectionResponse UOSEGenericGraphAssetSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
   // Make sure the pins are not on the same node
   if (A->GetOwningNode() == B->GetOwningNode())
   {
      return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorSameNode", "Can't connect node to itself"));
   }

   const UEdGraphPin *Out = A;
   const UEdGraphPin *In = B;

   UOSEGenericGraphEdNode* EdNode_Out = Cast<UOSEGenericGraphEdNode>(Out->GetOwningNode());
   UOSEGenericGraphEdNode* EdNode_In = Cast<UOSEGenericGraphEdNode>(In->GetOwningNode());

   if (EdNode_Out == nullptr || EdNode_In == nullptr)
   {
      return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinError", "Not a valid UOSEGenericGraphEdNode"));
   }

   //Determine if we can have cycles or not
   bool bAllowCycles = false;
   auto EdGraph = Cast<UOSEGenericGraphEdGraph>(Out->GetOwningNode()->GetGraph());
   if (EdGraph != nullptr)
   {
      bAllowCycles = EdGraph->GetGenericGraph()->bCanBeCyclical;
   }

   // check for cycles
   FNodeVisitorCycleChecker CycleChecker;
   if (!bAllowCycles && !CycleChecker.CheckForLoop(Out->GetOwningNode(), In->GetOwningNode()))
   {
      return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorCycle", "Can't create a graph cycle"));
   }

   FText ErrorMessage;
   if (!EdNode_Out->GenericGraphNode->CanCreateConnectionTo(EdNode_In->GenericGraphNode, EdNode_Out->GetOutputPin()->LinkedTo.Num(), ErrorMessage))
   {
      return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, ErrorMessage);
   }
   if (!EdNode_In->GenericGraphNode->CanCreateConnectionFrom(EdNode_Out->GenericGraphNode, EdNode_In->GetInputPin()->LinkedTo.Num(), ErrorMessage))
   {
      return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, ErrorMessage);
   }

   if (EdNode_Out->GenericGraphNode->GetGraph()->bEdgeTransitionEnabled)
   {
      return FPinConnectionResponse(CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE, LOCTEXT("PinConnect", "Connect nodes with edge"));
   }
   else
   {
      return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, LOCTEXT("PinConnect", "Connect nodes"));
   }
}

bool UOSEGenericGraphAssetSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
   // We don't actually care about the pin, we want the node that is being dragged between
   UOSEGenericGraphEdNode* NodeA = Cast<UOSEGenericGraphEdNode>(A->GetOwningNode());
   UOSEGenericGraphEdNode* NodeB = Cast<UOSEGenericGraphEdNode>(B->GetOwningNode());

   // Check that this edge doesn't already exist
   for (UEdGraphPin *TestPin : NodeA->GetOutputPin()->LinkedTo)
   {
      UEdGraphNode* ChildNode = TestPin->GetOwningNode();
      if (UOSEGenericGraphEdNodeEdge* EdNode_Edge = Cast<UOSEGenericGraphEdNodeEdge>(ChildNode))
      {
         ChildNode = EdNode_Edge->GetEndNode();
      }

      if (ChildNode == NodeB)
         return false;
   }

   if (NodeA && NodeB)
   {
      // Always create connections from node A to B, don't allow adding in reverse
      Super::TryCreateConnection(NodeA->GetOutputPin(), NodeB->GetInputPin());
      return true;
   }
   else
   {
      return false;
   }
}

bool UOSEGenericGraphAssetSchema::CreateAutomaticConversionNodeAndConnections(UEdGraphPin* A, UEdGraphPin* B) const
{
   UOSEGenericGraphEdNode* NodeA = Cast<UOSEGenericGraphEdNode>(A->GetOwningNode());
   UOSEGenericGraphEdNode* NodeB = Cast<UOSEGenericGraphEdNode>(B->GetOwningNode());

   // Are nodes and pins all valid?
   if (!NodeA || !NodeA->GetOutputPin() || !NodeB || !NodeB->GetInputPin())
      return false;

   UOSEGenericGraph* Graph = NodeA->GenericGraphNode->GetGraph();

   FVector2D InitPos((NodeA->NodePosX + NodeB->NodePosX) / 2, (NodeA->NodePosY + NodeB->NodePosY) / 2);

   FOSEGenericGraphAssetSchemaAction_NewEdge Action;
   Action.NodeTemplate = NewObject<UOSEGenericGraphEdNodeEdge>(NodeA->GetGraph());
   Action.NodeTemplate->SetEdge(NewObject<UOSEGenericGraphEdge>(Action.NodeTemplate, Graph->EdgeType));
   UOSEGenericGraphEdNodeEdge* EdgeNode = Cast<UOSEGenericGraphEdNodeEdge>(Action.PerformAction(NodeA->GetGraph(), nullptr, InitPos, false));

   // Always create connections from node A to B, don't allow adding in reverse
   EdgeNode->CreateConnections(NodeA, NodeB);

   return true;
}

class FConnectionDrawingPolicy* UOSEGenericGraphAssetSchema::CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const
{
   return new FOSEGenericGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj);
}

FLinearColor UOSEGenericGraphAssetSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
   return FColor::White;
}

void UOSEGenericGraphAssetSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
   const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakNodeLinks", "Break Node Links"));

   Super::BreakNodeLinks(TargetNode);
}

void UOSEGenericGraphAssetSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const
{
   const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakPinLinks", "Break Pin Links"));

   Super::BreakPinLinks(TargetPin, bSendsNodeNotifcation);
}

void UOSEGenericGraphAssetSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
   const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakSinglePinLink", "Break Pin Link"));

   Super::BreakSinglePinLink(SourcePin, TargetPin);
}

UEdGraphPin* UOSEGenericGraphAssetSchema::DropPinOnNode(UEdGraphNode* InTargetNode, const FName& InSourcePinName, const FEdGraphPinType& InSourcePinType, EEdGraphPinDirection InSourcePinDirection) const
{
   UOSEGenericGraphEdNode* EdNode = Cast<UOSEGenericGraphEdNode>(InTargetNode);
   switch (InSourcePinDirection)
   {
   case EGPD_Input:
      return EdNode->GetOutputPin();
   case EGPD_Output:
      return EdNode->GetInputPin();
   default:
      return nullptr;
   }
}

bool UOSEGenericGraphAssetSchema::SupportsDropPinOnNode(UEdGraphNode* InTargetNode, const FEdGraphPinType& InSourcePinType, EEdGraphPinDirection InSourcePinDirection, FText& OutErrorMessage) const
{
   return Cast<UOSEGenericGraphEdNode>(InTargetNode) != nullptr;
}

bool UOSEGenericGraphAssetSchema::IsCacheVisualizationOutOfDate(int32 InVisualizationCacheID) const
{
   return CurrentCacheRefreshID != InVisualizationCacheID;
}

int32 UOSEGenericGraphAssetSchema::GetCurrentVisualizationCacheID() const
{
   return CurrentCacheRefreshID;
}

void UOSEGenericGraphAssetSchema::ForceVisualizationCacheClear() const
{
   ++CurrentCacheRefreshID;
}

#undef LOCTEXT_NAMESPACE
