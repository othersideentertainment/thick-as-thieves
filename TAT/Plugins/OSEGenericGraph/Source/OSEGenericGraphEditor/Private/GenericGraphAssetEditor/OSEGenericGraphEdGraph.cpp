// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#include "GenericGraphAssetEditor/OSEGenericGraphEdGraph.h"
#include "OSEGenericGraphEditorPCH.h"
#include "OSEGenericGraph.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEdNode.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEdNodeEdge.h"

// ue
#include "Misc/DataValidation.h"

UOSEGenericGraphEdGraph::UOSEGenericGraphEdGraph()
{

}

UOSEGenericGraphEdGraph::~UOSEGenericGraphEdGraph()
{

}

void UOSEGenericGraphEdGraph::RebuildGenericGraph()
{
   LOG_INFO(TEXT("UOSEGenericGraphEdGraph::RebuildGenericGraph has been called"));

   UOSEGenericGraph* Graph = GetGenericGraph();

   Clear();

   for (int i = 0; i < Nodes.Num(); ++i)
   {
      if (UOSEGenericGraphEdNode* EdNode = Cast<UOSEGenericGraphEdNode>(Nodes[i]))
      {
         if (EdNode->GenericGraphNode == nullptr)
            continue;

         UOSEGenericGraphNode* GenericGraphNode = EdNode->GenericGraphNode;

         if (!GenericGraphNode->NodeId.IsValid())
         {
            GenericGraphNode->NodeId = FGuid::NewGuid();
         }

         NodeMap.Add(GenericGraphNode, EdNode);

         Graph->AllNodes.Add(GenericGraphNode);

         for (int PinIdx = 0; PinIdx < EdNode->Pins.Num(); ++PinIdx)
         {
            UEdGraphPin* Pin = EdNode->Pins[PinIdx];

            if (Pin->Direction != EEdGraphPinDirection::EGPD_Output)
               continue;

            for (int LinkToIdx = 0; LinkToIdx < Pin->LinkedTo.Num(); ++LinkToIdx)
            {
               UOSEGenericGraphNode* ChildNode = nullptr;
               if (UOSEGenericGraphEdNode* EdNode_Child = Cast<UOSEGenericGraphEdNode>(Pin->LinkedTo[LinkToIdx]->GetOwningNode()))
               {
                  ChildNode = EdNode_Child->GenericGraphNode;
               }
               else if (UOSEGenericGraphEdNodeEdge* EdNode_Edge = Cast<UOSEGenericGraphEdNodeEdge>(Pin->LinkedTo[LinkToIdx]->GetOwningNode()))
               {
                  UOSEGenericGraphEdNode* Child = EdNode_Edge->GetEndNode();;
                  if (Child != nullptr)
                  {
                     ChildNode = Child->GenericGraphNode;
                  }
               }

               if (ChildNode != nullptr)
               {
                  GenericGraphNode->ChildrenNodes.Add(ChildNode);

                  ChildNode->ParentNodes.Add(GenericGraphNode);
               }
               else
               {
                  LOG_ERROR(TEXT("UOSEGenericGraphEdGraph::RebuildGenericGraph can't find child node"));
               }
            }
         }
      }
      else if (UOSEGenericGraphEdNodeEdge* EdgeNode = Cast<UOSEGenericGraphEdNodeEdge>(Nodes[i]))
      {
         UOSEGenericGraphEdNode* StartNode = EdgeNode->GetStartNode();
         UOSEGenericGraphEdNode* EndNode = EdgeNode->GetEndNode();
         UOSEGenericGraphEdge* Edge = EdgeNode->GenericGraphEdge;

         if (StartNode == nullptr || EndNode == nullptr || Edge == nullptr)
         {
            LOG_ERROR(TEXT("UOSEGenericGraphEdGraph::RebuildGenericGraph add edge failed."));
            continue;
         }

         EdgeMap.Add(Edge, EdgeNode);

         Edge->Graph = Graph;
         Edge->Rename(nullptr, Graph, REN_DontCreateRedirectors | REN_DoNotDirty);
         Edge->StartNode = StartNode->GenericGraphNode;
         Edge->EndNode = EndNode->GenericGraphNode;
         Edge->StartNode->Edges.Add(Edge->EndNode, Edge);
      }
   }

   for (int i = 0; i < Graph->AllNodes.Num(); ++i)
   {
      UOSEGenericGraphNode* Node = Graph->AllNodes[i];
      if (Node->ParentNodes.Num() == 0)
      {
         Graph->RootNodes.Add(Node);

         SortNodes(Node);
      }

      Node->Graph = Graph;
      Node->Rename(nullptr, Graph, REN_DontCreateRedirectors | REN_DoNotDirty);
   }

   Graph->RootNodes.Sort([&](const UOSEGenericGraphNode& L, const UOSEGenericGraphNode& R)
   {
      UOSEGenericGraphEdNode* EdNode_LNode = NodeMap[&L];
      UOSEGenericGraphEdNode* EdNode_RNode = NodeMap[&R];
      return EdNode_LNode->NodePosX < EdNode_RNode->NodePosX;
   });
}

UOSEGenericGraph* UOSEGenericGraphEdGraph::GetGenericGraph() const
{
   return CastChecked<UOSEGenericGraph>(GetOuter());
}

bool UOSEGenericGraphEdGraph::Modify(bool bAlwaysMarkDirty /*= true*/)
{
   bool Rtn = Super::Modify(bAlwaysMarkDirty);

   GetGenericGraph()->Modify();

   for (int32 i = 0; i < Nodes.Num(); ++i)
   {
      Nodes[i]->Modify();
   }

   return Rtn;
}

void UOSEGenericGraphEdGraph::Clear()
{
   UOSEGenericGraph* Graph = GetGenericGraph();

   Graph->ClearGraph();
   NodeMap.Reset();
   EdgeMap.Reset();

   for (int i = 0; i < Nodes.Num(); ++i)
   {
      if (UOSEGenericGraphEdNode* EdNode = Cast<UOSEGenericGraphEdNode>(Nodes[i]))
      {
         UOSEGenericGraphNode* GenericGraphNode = EdNode->GenericGraphNode;
         if (GenericGraphNode)
         {
            GenericGraphNode->ParentNodes.Reset();
            GenericGraphNode->ChildrenNodes.Reset();
            GenericGraphNode->Edges.Reset();
         }
      }
   }
}

void UOSEGenericGraphEdGraph::SortNodes(UOSEGenericGraphNode* RootNode)
{
   int Level = 0;
   TArray<UOSEGenericGraphNode*> CurrLevelNodes = { RootNode };
   TArray<UOSEGenericGraphNode*> NextLevelNodes;
   TSet<UOSEGenericGraphNode*> Visited;

   while (CurrLevelNodes.Num() != 0)
   {
      int32 LevelWidth = 0;
      for (int i = 0; i < CurrLevelNodes.Num(); ++i)
      {
         UOSEGenericGraphNode* Node = CurrLevelNodes[i];
         Visited.Add(Node);

         auto Comp = [&](const UOSEGenericGraphNode& L, const UOSEGenericGraphNode& R)
         {
            UOSEGenericGraphEdNode* EdNode_LNode = NodeMap[&L];
            UOSEGenericGraphEdNode* EdNode_RNode = NodeMap[&R];
            return EdNode_LNode->NodePosX < EdNode_RNode->NodePosX;
         };

         Node->ChildrenNodes.Sort(Comp);
         Node->ParentNodes.Sort(Comp);

         for (int j = 0; j < Node->ChildrenNodes.Num(); ++j)
         {
            UOSEGenericGraphNode* ChildNode = Node->ChildrenNodes[j];
            if(!Visited.Contains(ChildNode))
               NextLevelNodes.Add(Node->ChildrenNodes[j]);
         }
      }

      CurrLevelNodes = NextLevelNodes;
      NextLevelNodes.Reset();
      ++Level;
   }
}

void UOSEGenericGraphEdGraph::PostEditUndo()
{
   Super::PostEditUndo();

   NotifyGraphChanged();
}

#if WITH_EDITOR
EDataValidationResult UOSEGenericGraphEdGraph::IsDataValid(FDataValidationContext& context) const
{
   for (int i = 0; i < Nodes.Num() - 1; ++i)
   {
      if (UOSEGenericGraphEdNode* EdNodeA = Cast<UOSEGenericGraphEdNode>(Nodes[i]))
      {
         if (!EdNodeA->GenericGraphNode)
         {
            context.AddError(FText::FormatOrdered(
               INVTEXT("Empty node found: {0}"),
               FText::FromString(EdNodeA->GetName())));
            continue;
         }

         if (!EdNodeA->GenericGraphNode->NodeId.IsValid())
         {
            context.AddError(FText::FormatOrdered(
               INVTEXT("Found node with invalid ID: {0}"),
               FText::FromString(EdNodeA->GenericGraphNode->GetName())));
         }

         if (EdNodeA->GenericGraphNode->GetOuter() == EdNodeA)
         {
            // check for an issue where the graph was not being rebuilt before it got saved
            // in this case, the outer of the generic node was not correctly reassigned from the editor node to a graph node
            context.AddError(FText::FormatOrdered(
               INVTEXT("Graph node outer is editor node -- this graph may not have been rebuilt correctly: {0}"),
               FText::FromString(EdNodeA->GenericGraphNode->GetName())
            ));
         }

         // check for duplicate nodes
         for (int j = i + 1; j < Nodes.Num(); ++j)
         {
            UOSEGenericGraphEdNode* EdNodeB = Cast<UOSEGenericGraphEdNode>(Nodes[j]);
            if (!EdNodeB)
            {
               // TODO check likely edge node
               continue;
            }

            if (EdNodeA == EdNodeB || 
               EdNodeA->GenericGraphNode == EdNodeB->GenericGraphNode ||
               EdNodeA->GetUniqueID() == EdNodeB->GetUniqueID())
            {
               context.AddError(FText::FormatOrdered(
                  INVTEXT("Duplicate nodes found: {0} {1}"),
                  FText::FromString(EdNodeA->GetName()),
                  FText::FromString(EdNodeB->GetName())));
            }
         }

         // check for bad pins
         for (int PinIdx = 0; PinIdx < EdNodeA->Pins.Num(); ++PinIdx)
         {
            UEdGraphPin* Pin = EdNodeA->Pins[PinIdx];

            if (Pin->Direction != EEdGraphPinDirection::EGPD_Output)
            {
               continue;
            }

            for (int LinkToIdx = 0; LinkToIdx < Pin->LinkedTo.Num(); ++LinkToIdx)
            {
               UOSEGenericGraphNode* ChildNode = nullptr;
               if (UOSEGenericGraphEdNode* EdNode_Child = Cast<UOSEGenericGraphEdNode>(Pin->LinkedTo[LinkToIdx]->GetOwningNode()))
               {
                  ChildNode = EdNode_Child->GenericGraphNode;
               }
               else if (UOSEGenericGraphEdNodeEdge* EdNode_Edge = Cast<UOSEGenericGraphEdNodeEdge>(Pin->LinkedTo[LinkToIdx]->GetOwningNode()))
               {
                  UOSEGenericGraphEdNode* Child = EdNode_Edge->GetEndNode();;
                  if (Child != nullptr)
                  {
                     ChildNode = Child->GenericGraphNode;
                  }
               }

               if (ChildNode == nullptr)
               {
                  context.AddError(FText::FormatOrdered(
                     INVTEXT("No child found after node: {0}"), 
                     FText::FromString(EdNodeA->GetName())));
               }
            }
         }
      }
      else if (UOSEGenericGraphEdNodeEdge* EdNodeEdge = Cast<UOSEGenericGraphEdNodeEdge>(Nodes[i]))
      {
         if (!EdNodeEdge->GenericGraphEdge)
         {
            context.AddError(FText::FormatOrdered(
               INVTEXT("Empty edge found: {0}"),
               FText::FromString(EdNodeEdge->GetName())));
            continue;
         }

         if (EdNodeEdge->GenericGraphEdge->GetOuter() == EdNodeEdge)
         {
            // check for an issue where the graph was not being rebuilt before it got saved
            // in this case, the outer of the generic node was not correctly reassigned from the editor node to a graph node
            context.AddError(FText::FormatOrdered(
               INVTEXT("Graph edge outer is editor edge -- this graph may not have been rebuilt correctly: {0}"),
               FText::FromString(EdNodeEdge->GenericGraphEdge->GetName())
            ));
         }
      }
   }
   return context.GetNumErrors() + context.GetNumWarnings() > 0 ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif
