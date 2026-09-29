// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEGenericGraphNodeHandle.h"
#include "OSEGenericGraph.h"
#include "OSEGenericGraphNode.h"

#define LOCTEXT_NAMESPACE "OSEGenericGraphNodeHandle"

FOSEGenericGraphNodeHandle::FOSEGenericGraphNodeHandle(const UOSEGenericGraphNode* InNode)
{
   if (InNode != nullptr)
   {
      Graph = InNode->Graph;
      NodeId = InNode->NodeId;
   }
}

FOSEGenericGraphNodeHandle::FOSEGenericGraphNodeHandle(UOSEGenericGraph* InGraph, const FGuid& InNodeId)
   : Graph(InGraph)
   , NodeId(InNodeId)
{
}

UOSEGenericGraphNode* FOSEGenericGraphNodeHandle::GetNode() const
{
   return (Graph != nullptr) ? Graph->GetNodeByGuid(NodeId) : nullptr;
}

FString FOSEGenericGraphNodeHandle::ToDebugString() const
{
   return (Graph != nullptr)
      ? FString::Printf(TEXT("NodeHandle(Graph=%s, NodeId=%s)"), *Graph->GetName(), *NodeId.ToString(EGuidFormats::DigitsWithHyphensLower))
      : FString::Printf(TEXT("NodeHandle(Graph=NULL, NodeId=%s)"), *NodeId.ToString(EGuidFormats::DigitsWithHyphensLower));
}

bool FOSEGenericGraphNodeHandle::operator==(const FOSEGenericGraphNodeHandle& Other) const
{
   return Graph == Other.Graph && NodeId == Other.NodeId;
}

// static
FOSEGenericGraphNodeHandle UOSEGenericGraphNodeHandleFunctionLibrary::MakeGenericGraphNodeHandle(UOSEGenericGraphNode* Node)
{
   return FOSEGenericGraphNodeHandle{ Node };
}

#undef LOCTEXT_NAMESPACE
