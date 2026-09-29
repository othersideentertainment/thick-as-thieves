// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#include "OSEGenericGraphEdge.h"
#include "OSEGenericGraphNode.h"

UOSEGenericGraph* UOSEGenericGraphEdge::GetGraph() const
{
   return Graph;
}

UOSEGenericGraphNode* UOSEGenericGraphEdge::GetOtherNode(UOSEGenericGraphNode* Node) const
{
   if (Node == StartNode)
   {
      return EndNode;
   }
   else if (Node == EndNode)
   {
      return StartNode;
   }
   return nullptr;
}

FString UOSEGenericGraphEdge::GetEdgeDebugName() const
{
#if WITH_EDITOR
   const FText displayTitle = GetEdgeDisplayTitle();
   const FString name = displayTitle.IsEmpty() ? GetName() : displayTitle.ToString();
   if (StartNode != nullptr || EndNode != nullptr)
   {
      const FString startNodeName = (StartNode != nullptr) ? StartNode->GetNodeDebugName() : FString();
      const FString endNodeName = (EndNode != nullptr) ? EndNode->GetNodeDebugName() : FString();
      return FString::Printf(TEXT("%s (Connects '%s' -> '%s')"), *name,
         (!startNodeName.IsEmpty() ? *startNodeName : TEXT("NULL")),
         (!endNodeName.IsEmpty() ? *endNodeName : TEXT("NULL")));
   }
   return name;
#else
   return GetName();
#endif
}

#if WITH_EDITOR
const FSlateBrush* UOSEGenericGraphEdge::GetEdgeIcon() const
{
   return FAppStyle::GetBrush("Graph.TransitionNode.Icon");
}
#endif
