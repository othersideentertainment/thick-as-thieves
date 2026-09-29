// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEGenericGraphSoftNodeHandle.h"
#include "OSEGenericGraph.h"
#include "OSEGenericGraphNode.h"
#include "Engine/AssetManager.h"

#define LOCTEXT_NAMESPACE "OSEGenericGraphSoftNodeHandle"

FOSEGenericGraphSoftNodeHandle::FOSEGenericGraphSoftNodeHandle(const UOSEGenericGraphNode* InNode)
{
   if (InNode != nullptr)
   {
      Graph = InNode->Graph;
      NodeId = InNode->NodeId;
   }
}

FOSEGenericGraphSoftNodeHandle::FOSEGenericGraphSoftNodeHandle(const FOSEGenericGraphNodeHandle& InHandle)
   : Graph(InHandle.Graph)
   , NodeId(InHandle.NodeId)
{
}

FOSEGenericGraphSoftNodeHandle::FOSEGenericGraphSoftNodeHandle(const TSoftObjectPtr<UOSEGenericGraph>& InGraph, const FGuid& InNodeId)
   : Graph(InGraph)
   , NodeId(InNodeId)
{
}

FOSEGenericGraphNodeHandle FOSEGenericGraphSoftNodeHandle::LoadSynchronous() const
{
   return FOSEGenericGraphNodeHandle{ Graph.LoadSynchronous(), NodeId };
}

bool FOSEGenericGraphSoftNodeHandle::RequestAsyncLoad(const TFunction<void(FOSEGenericGraphNodeHandle)>& OnLoadedCallback) const
{
   check(OnLoadedCallback != nullptr);
   if (Graph.IsNull())
   {
      return false;
   }
   auto Callback = [OnLoadedCallback, graph = Graph, nodeId = NodeId]()
   {
      OnLoadedCallback(FOSEGenericGraphNodeHandle{ graph.Get(), nodeId });
   };
   return UAssetManager::GetStreamableManager().RequestAsyncLoad(Graph.ToSoftObjectPath(), Callback) != nullptr;
}

FString FOSEGenericGraphSoftNodeHandle::ToDebugString() const
{
   return (!Graph.IsNull())
      ? FString::Printf(TEXT("SoftNodeHandle(Graph=%s, NodeId=%s)"), *Graph.ToString(), *NodeId.ToString(EGuidFormats::DigitsWithHyphensLower))
      : FString::Printf(TEXT("SoftNodeHandle(Graph=NULL, NodeId=%s)"), *NodeId.ToString(EGuidFormats::DigitsWithHyphensLower));
}

bool FOSEGenericGraphSoftNodeHandle::operator==(const FOSEGenericGraphSoftNodeHandle& Other) const
{
   return Graph == Other.Graph && NodeId == Other.NodeId;
}

// static
FOSEGenericGraphSoftNodeHandle UOSEGenericGraphSoftNodeHandleFunctionLibrary::MakeGenericGraphSoftNodeHandleFromNode(UOSEGenericGraphNode* Node)
{
   return FOSEGenericGraphSoftNodeHandle{ Node };
}

// static
FOSEGenericGraphSoftNodeHandle UOSEGenericGraphSoftNodeHandleFunctionLibrary::MakeGenericGraphSoftNodeHandleFromHandle(const FOSEGenericGraphNodeHandle& Handle)
{
   return FOSEGenericGraphSoftNodeHandle{ Handle };
}

#undef LOCTEXT_NAMESPACE
