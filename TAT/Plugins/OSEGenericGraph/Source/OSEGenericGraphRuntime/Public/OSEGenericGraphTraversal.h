// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once
#include "OSEGenericGraph.h"
#include "Engine/CancellableAsyncAction.h"
#include "OSEGenericGraphTraversal.generated.h"

class UOSEGenericGraphNode;
class UOSEGenericGraphEdge;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOSEGenericGraphNodeEdgeDelegate, UOSEGenericGraphNode*, Node, UOSEGenericGraphEdge*, Edge);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOSEGenericGraphNodeDelegate, UOSEGenericGraphNode*, Node);

UCLASS(Abstract)
class OSEGENERICGRAPHRUNTIME_API UOSEGenericGraphTraversalBase : public UCancellableAsyncAction
{
   GENERATED_BODY()

protected:
   /// Subclasses should implement this to do the graph search
   virtual void SearchGraph() {}

   /// Subclasses should make a static BlueprintCallable UFUNCTION to create the node that calls this
   template<typename T>
   static T* MakeGraphSearchAsyncAction(const UObject* WorldContextObject, UOSEGenericGraph* InGraph, EOSEGenericGraphSearchMode InSearchMode, UOSEGenericGraphNode* InStartNode)
   {
      check(T::StaticClass()->IsChildOf(UOSEGenericGraphTraversalBase::StaticClass()));
      UOSEGenericGraphTraversalBase* BlueprintNode = NewObject<T>();
      BlueprintNode->Graph = InGraph;
      BlueprintNode->SearchMode = InSearchMode;
      BlueprintNode->StartNode = InStartNode;
      // Register with the game instance to avoid being garbage collected
      BlueprintNode->RegisterWithGameInstance(WorldContextObject);
      return CastChecked<T>(BlueprintNode);
   }

public:
   // From UBlueprintAsyncActionBase
   virtual void Activate() override
   {
      SearchGraph();
      // Unregister from the game instance so we can be garbage collected
      SetReadyToDestroy();
   }

   // From UCancellableAsyncAction
   virtual void Cancel() override { bWasCancelled = true; Super::Cancel(); }
   virtual bool ShouldBroadcastDelegates() const override { return !bWasCancelled && Super::ShouldBroadcastDelegates(); }

   /// Stops graph traversal
   UFUNCTION(BlueprintCallable, Category = "Generic Graph", Meta = (CompactNodeTitle = "Break"))
   void Break() { Cancel(); }

protected:
   UPROPERTY(Transient)
   TObjectPtr<UOSEGenericGraph> Graph;

   EOSEGenericGraphSearchMode SearchMode = EOSEGenericGraphSearchMode::BreadthFirstSearch;

   UPROPERTY(Transient)
   TObjectPtr<UOSEGenericGraphNode> StartNode;

   bool bWasCancelled = false;
};

UCLASS()
class OSEGENERICGRAPHRUNTIME_API UOSEGenericGraphNodeTraversal : public UOSEGenericGraphTraversalBase
{
   GENERATED_BODY()

protected:
   virtual void SearchGraph() override;

public:
   /// Traverses nodes in a generic graph.
   /// If a start node is specified, this will start there and only visit nodes connected to that one.
   /// If no start node is specified, all root nodes will be included (and any nodes they connect to).
   /// To cancel graph traversal early, in the ForEachNode handler, call the Break function on the Async Action object.
   ///
   /// Traverses nodes in a generic graph. Each node will be visited exactly once.
   /// To stop traversing the graph before completion, in the ForEachNode handler, call the Break function on the Async Action object.
   UFUNCTION(BlueprintCallable, Category = "Generic Graph", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
   static UOSEGenericGraphNodeTraversal* TraverseGenericGraphNodes(const UObject* WorldContextObject, UOSEGenericGraph* InGraph, EOSEGenericGraphSearchMode InSearchMode,
      bool TraverseBackwards, UOSEGenericGraphNode* InStartNode)
   {
      UOSEGenericGraphNodeTraversal* Node = MakeGraphSearchAsyncAction<UOSEGenericGraphNodeTraversal>(WorldContextObject, InGraph, InSearchMode, InStartNode);
      Node->Backwards = TraverseBackwards;
      return Node;
   }

   UPROPERTY(BlueprintAssignable)
   FOSEGenericGraphNodeDelegate ForEachNode;

   UPROPERTY(BlueprintAssignable)
   FOSEGenericGraphNodeDelegate Complete;

protected:
   UPROPERTY(Transient)
   bool Backwards = false;

   UPROPERTY(Transient)
   TObjectPtr<UOSEGenericGraphNode> ResultNode;
};

UCLASS()
class OSEGENERICGRAPHRUNTIME_API UOSEGenericGraphNodeEdgePairsTraversal : public UOSEGenericGraphTraversalBase
{
   GENERATED_BODY()

protected:
   virtual void SearchGraph() override;

public:
   /// Traverses node/edge pairs in a generic graph.
   /// If a start node is specified, this will start there and only visit that node and any connecting nodes.
   /// If no start node is specified, all root nodes will be included (and any nodes they connect to).
   /// Note that nodes will be iterated on once for each edge leading to it - if two edges lead to a node, that node will appear twice.
   /// Nodes without any nodes leading to them (root nodes) will appear exactly once and will not have a valid edge node.
   /// To stop traversing the graph before completion, in the ForEachNodeEdgePair handler, call the Break function on the Async Action object.
   /// NB. This function only works on graphs with transition/edge nodes enabled
   UFUNCTION(BlueprintCallable, Category = "Generic Graph", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
   static UOSEGenericGraphNodeEdgePairsTraversal* TraverseGenericGraphNodeEdgePairs(const UObject* WorldContextObject, UOSEGenericGraph* InGraph, EOSEGenericGraphSearchMode InSearchMode, UOSEGenericGraphNode* InStartNode = nullptr)
   {
      return MakeGraphSearchAsyncAction<UOSEGenericGraphNodeEdgePairsTraversal>(WorldContextObject, InGraph, InSearchMode, InStartNode);
   }

   UPROPERTY(BlueprintAssignable)
   FOSEGenericGraphNodeEdgeDelegate ForEachNodeEdgePair;

   UPROPERTY(BlueprintAssignable)
   FOSEGenericGraphNodeEdgeDelegate Complete;

protected:
   UPROPERTY(Transient)
   TObjectPtr<UOSEGenericGraphNode> ResultNode;
   UPROPERTY(Transient)
   TObjectPtr<UOSEGenericGraphEdge> ResultFromEdge;
};
