// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEGenericGraphTraversal.h"
#include "OSEGenericGraph.h"

DEFINE_LOG_CATEGORY_STATIC(LogOSEGenericGraphTraversal, Log, All);

void UOSEGenericGraphNodeTraversal::SearchGraph()
{
   if (Graph != nullptr && ForEachNode.IsBound())
   {
      auto TraverseCallback = [this](UOSEGenericGraphNode* Node)
      {
         // This will be our return value if the search is cancelled during this iteration
         ResultNode = Node;
         if (ShouldBroadcastDelegates())
         {
            ForEachNode.Broadcast(Node);
         }
         return bWasCancelled ? UOSEGenericGraph::TraverseBreak : UOSEGenericGraph::TraverseContinue;
      };

      if (Backwards)
      {
         if (StartNode != nullptr)
         {
            // Always visit the starting node to keep things consistent with the forward traverse behavior
            constexpr bool VisitBaseNode = true;
            Graph->TraverseNodesBackward(SearchMode, StartNode, TraverseCallback, VisitBaseNode);
         }
         else
         {
            UE_LOG(LogOSEGenericGraphTraversal, Error, TEXT("Generic graph node traversal requires a start node when traversing backwards!"));
         }
      }
      else
      {
         Graph->TraverseNodes(SearchMode, TraverseCallback, StartNode);
      }
   }

   if (Complete.IsBound())
   {
      Complete.Broadcast(ResultNode);
   }
}

void UOSEGenericGraphNodeEdgePairsTraversal::SearchGraph()
{
   if (Graph != nullptr && ForEachNodeEdgePair.IsBound())
   {
      Graph->TraverseNodeEdgePairs(SearchMode,
         [this](UOSEGenericGraphNode* Node, UOSEGenericGraphEdge* Edge)
         {
            // This will be our return value if the search is cancelled during this iteration
            ResultNode = Node;
            ResultFromEdge = Edge;
            if (ShouldBroadcastDelegates())
            {
               ForEachNodeEdgePair.Broadcast(Node, Edge);
            }
            return bWasCancelled ? UOSEGenericGraph::TraverseBreak : UOSEGenericGraph::TraverseContinue;
         }, StartNode);
   }

   if (Complete.IsBound())
   {
      Complete.Broadcast(ResultNode, ResultFromEdge);
   }
}

