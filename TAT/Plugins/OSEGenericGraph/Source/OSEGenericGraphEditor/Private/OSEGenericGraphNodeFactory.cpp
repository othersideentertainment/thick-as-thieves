// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#include "OSEGenericGraphNodeFactory.h"
#include <EdGraph/EdGraphNode.h>
#include "GenericGraphAssetEditor/SOSEGenericGraphEdNodeEdge.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEdNode.h"
#include "GenericGraphAssetEditor/SOSEGenericGraphEdNode.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEdNodeEdge.h"

TSharedPtr<class SGraphNode> FOSEGenericGraphPanelNodeFactory::CreateNode(UEdGraphNode* Node) const
{
   if (UOSEGenericGraphEdNode* EdNode_GraphNode = Cast<UOSEGenericGraphEdNode>(Node))
   {
      return SNew(SOSEGenericGraphEdNode, EdNode_GraphNode);
   }
   else if (UOSEGenericGraphEdNodeEdge* EdNode_Edge = Cast<UOSEGenericGraphEdNodeEdge>(Node))
   {
      return SNew(SOSEGenericGraphEdNodeEdge, EdNode_Edge);
   }
   return nullptr;
}

