// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#include "OSEGenericGraphNode.h"
#include "OSEGenericGraph.h"

#define LOCTEXT_NAMESPACE "OSEGenericGraphNode"

UOSEGenericGraphNode::UOSEGenericGraphNode()
{
#if WITH_EDITORONLY_DATA
   CompatibleGraphType = UOSEGenericGraph::StaticClass();
#endif
}

UOSEGenericGraphEdge* UOSEGenericGraphNode::GetEdge(UOSEGenericGraphNode* ChildNode) const
{
   UOSEGenericGraphEdge* const* EdgePtr = Edges.Find(ChildNode);
   return (EdgePtr != nullptr) ? *EdgePtr : nullptr;
}

FString UOSEGenericGraphNode::GetNodeDebugName() const
{
#if WITH_EDITOR
   const FText displayTitle = GetNodeListViewTitle();
   return displayTitle.IsEmpty() ? GetName() : displayTitle.ToString();
#else
   return GetName();
#endif
}

#if WITH_EDITOR
FLinearColor UOSEGenericGraphNode::GetBackgroundColor() const
{
   return FLinearColor::Black;
}

const FSlateBrush* UOSEGenericGraphNode::GetNodeIcon() const
{
   return FAppStyle::GetBrush(TEXT("BTEditor.Graph.BTNode.Icon"));
}

bool UOSEGenericGraphNode::CanCreateConnection(UOSEGenericGraphNode* Other, FText& ErrorMessage)
{
   return true;
}

bool UOSEGenericGraphNode::CanCreateConnectionTo(UOSEGenericGraphNode* Other, int32 NumberOfChildrenNodes, FText& ErrorMessage)
{
   if (ChildrenLimitType == EOSEGenericGraphNodeLimit::Limited && NumberOfChildrenNodes >= ChildrenLimit)
   {
      ErrorMessage = FText::FromString("Children limit exceeded");
      return false;
   }

   return CanCreateConnection(Other, ErrorMessage);
}

bool UOSEGenericGraphNode::CanCreateConnectionFrom(UOSEGenericGraphNode* Other, int32 NumberOfParentNodes, FText& ErrorMessage)
{
   if (ParentLimitType == EOSEGenericGraphNodeLimit::Limited && NumberOfParentNodes >= ParentLimit)
   {
      ErrorMessage = FText::FromString("Parent limit exceeded");
      return false;
   }

   return true;
}


#endif

bool UOSEGenericGraphNode::IsLeafNode() const
{
   return ChildrenNodes.Num() == 0;
}

UOSEGenericGraph* UOSEGenericGraphNode::GetGraph() const
{
   return Graph;
}

#undef LOCTEXT_NAMESPACE
