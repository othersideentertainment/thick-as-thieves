// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#include "OSEGenericGraph.h"
#include "OSEGenericGraphRuntimePCH.h"
#include "Engine/Engine.h"
#include "Misc/DataValidation.h"
#include "OSEGenericGraphNode.h"

#define LOCTEXT_NAMESPACE "OSEGenericGraph"

template<typename T, uint32 InlineVisitedSlots = 64, uint32 InlinePendingSlots = 16>
struct TGenericGraphTraversalState
{
   EOSEGenericGraphSearchMode Mode;
   TSet<T, DefaultKeyFuncs<T>, TInlineSetAllocator<InlineVisitedSlots>> Visited;
   TArray<T, TInlineAllocator<InlinePendingSlots>> Pending;

   explicit TGenericGraphTraversalState(EOSEGenericGraphSearchMode InMode) : Mode(InMode) {}

   inline void MarkVisited(T value) { Visited.Add(value); }
   inline bool IsVisited(T value) const { return Visited.Contains(value); }
   inline T PopPending() { return Pending.Pop(); }
   inline void PushPending(T Value)
   {
      if (Mode == EOSEGenericGraphSearchMode::BreadthFirstSearch)
      {
         Pending.Insert(Value, 0);
      }
      else if (Mode == EOSEGenericGraphSearchMode::DepthFirstSearch)
      {
         Pending.Add(Value);
      }
      else
      {
         checkf(false, TEXT("Unsupported graph search mode %d"), static_cast<int32>(Mode));
      }
   }
};

struct FNodeEdgePair
{
   UOSEGenericGraphNode* Node = nullptr;
   UOSEGenericGraphEdge* Edge = nullptr;

   bool operator==(const FNodeEdgePair& Rhs) const { return Node == Rhs.Node && Edge == Rhs.Edge; }
   friend uint32 GetTypeHash(const FNodeEdgePair& Pair) { return HashCombine(GetTypeHash(Pair.Node), GetTypeHash(Pair.Edge)); }
};

UOSEGenericGraph::UOSEGenericGraph()
{
   NodeTypes = { UOSEGenericGraphNode::StaticClass() };
   EdgeType = UOSEGenericGraphEdge::StaticClass();
}

#if WITH_EDITOR
EDataValidationResult UOSEGenericGraph::IsDataValid(FDataValidationContext& Context) const
{
   const EDataValidationResult BaseResult = Super::IsDataValid(Context);

   if (_CheckForDuplicateNodes())
   {
      Context.AddError(FText::FromString("Found duplicate nodes"));
   }

   // Verify that graphs that can't be cyclical aren't cyclical
   if (!bCanBeCyclical)
   {
      FindCycles([&Context](TArrayView<UOSEGenericGraphNode*> Cycle)
      {
         TArray<FText> CycleDesc;
         CycleDesc.SetNum(Cycle.Num());
         for (int32 i = 0; i < Cycle.Num(); i++)
         {
            CycleDesc[i] = FText::Format(INVTEXT("'{0}'"), Cycle[i]->GetNodeListViewTitle());
         }
         Context.AddError(FText::Format(LOCTEXT("CyclicGraphError", "Graph has bCanBeCyclical=false, but a cycle was detected:\n{0}"),
            FText::Join(FText::FromString(TEXT("\n-> ")), CycleDesc)));
      });
   }

   for (const UOSEGenericGraphNode* Node : AllNodes)
   {
      if (!ensure(Node != nullptr))
      {
         continue;
      }

      // Make sure all nodes have types that match the graph's NodeTypes array
      bool IsValidNodeType = false;
      for (TSubclassOf<UOSEGenericGraphNode> NodeType : NodeTypes)
      {
         if (Node->GetClass()->IsChildOf(NodeType))
         {
            IsValidNodeType = true;
            break;
         }
      }
      if (!IsValidNodeType)
      {
         Context.AddError(FText::FromString(FString::Printf(TEXT("Graph node '%s' is not a valid type for this graph"), *Node->GetNodeDebugName())));
      }

      // Make sure all edges have types that match the graph's EdgeType
      for (const auto& Pair : Node->Edges)
      {
         if (Pair.Value != nullptr && !Pair.Value->GetClass()->IsChildOf(EdgeType))
         {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Graph connection '%s' -> '%s' has edge '%s' which is not a subclass of this graph's edge type"),
               *Node->GetNodeDebugName(), *Pair.Key->GetNodeDebugName(), *Pair.Value->GetEdgeDebugName())));
         }
      }
   }

   if (const UEdGraph* ConstEdGraph = EdGraph)
   {
      ConstEdGraph->IsDataValid(Context);
   }

   return CombineDataValidationResults(BaseResult,
      (Context.GetNumErrors() + Context.GetNumWarnings() > 0) ? EDataValidationResult::Invalid : EDataValidationResult::Valid);
}
#endif // WITH_EDITOR

void UOSEGenericGraph::TraverseNodes(EOSEGenericGraphSearchMode SearchMode, TFunctionRef<bool(UOSEGenericGraphNode*)> Callback, UOSEGenericGraphNode* StartNode) const
{
   TGenericGraphTraversalState<UOSEGenericGraphNode*> Search{ SearchMode };

   if (StartNode != nullptr)
   {
      if (!ensure(AllNodes.Contains(StartNode)))
      {
         return;
      }
      Search.Pending.Add(StartNode);
      Search.Visited.Add(StartNode);
   }
   else if (RootNodes.Num() > 0)
   {
      for (UOSEGenericGraphNode* Node : RootNodes)
      {
         Search.Pending.Add(Node);
         Search.Visited.Add(Node);
      }
   }

   constexpr bool bAllowShrinking = false;

   while (Search.Pending.Num() > 0)
   {
      UOSEGenericGraphNode* ThisNode = Search.PopPending();

      // Fire the callback for the current node
      const bool ContinueIterating = Callback(ThisNode);
      if (!ContinueIterating)
      {
         return;
      }

      // Find the next level of child nodes to iterate over
      if (ensure(ThisNode != nullptr))
      {
         for (UOSEGenericGraphNode* ChildNode : ThisNode->ChildrenNodes)
         {
            if (!Search.Visited.Contains(ChildNode))
            {
               Search.PushPending(ChildNode);
               Search.Visited.Add(ChildNode);
            }
         }
      }
   }
}

void UOSEGenericGraph::TraverseNodeEdgePairs(EOSEGenericGraphSearchMode SearchMode, TFunctionRef<bool(UOSEGenericGraphNode*, UOSEGenericGraphEdge*)> Callback, UOSEGenericGraphNode* StartNode) const
{
   TGenericGraphTraversalState<FNodeEdgePair> Search{ SearchMode };

   if (StartNode != nullptr)
   {
      if (!ensure(AllNodes.Contains(StartNode)))
      {
         return;
      }
      const FNodeEdgePair RootPair{ StartNode, nullptr };
      Search.Pending.Add(RootPair);
      Search.Visited.Add(RootPair);
   }
   else if (RootNodes.Num() > 0)
   {
      for (UOSEGenericGraphNode* Node : RootNodes)
      {
         const FNodeEdgePair RootPair{ Node, nullptr };
         Search.Pending.Add(RootPair);
         Search.Visited.Add(RootPair);
      }
   }

   constexpr bool bAllowShrinking = false;

   while (Search.Pending.Num() != 0)
   {
      const FNodeEdgePair Pair = Search.PopPending();

      // Fire the callback for the current node
      const bool ContinueIterating = Callback(Pair.Node, Pair.Edge);
      if (!ContinueIterating)
      {
         return;
      }

      // Find the next level of child nodes to iterate over
      for (UOSEGenericGraphNode* ChildNode : Pair.Node->ChildrenNodes)
      {
         const FNodeEdgePair ChildPair{ ChildNode, Pair.Node->GetEdge(ChildNode) };
         if (!Search.Visited.Contains(ChildPair))
         {
            Search.PushPending(ChildPair);
            Search.Visited.Add(ChildPair);
         }
      }
   }
}

void UOSEGenericGraph::TraverseNodesBackward(EOSEGenericGraphSearchMode SearchMode, UOSEGenericGraphNode* BaseNode, TFunctionRef<bool(UOSEGenericGraphNode*)> Callback, bool VisitBaseNode) const
{
   if (BaseNode == nullptr)
   {
      return;
   }

   TGenericGraphTraversalState<UOSEGenericGraphNode*> Search{ SearchMode };

   if (!ensure(AllNodes.Contains(BaseNode)))
   {
      return;
   }
   Search.Visited.Add(BaseNode);

   // Fire the callback for the base node if requested
   if (VisitBaseNode)
   {
      const bool ContinueIterating = Callback(BaseNode);
      if (!ContinueIterating)
      {
         return;
      }
   }

   // Start with all parent nodes
   for (UOSEGenericGraphNode* ParentNode : BaseNode->ParentNodes)
   {
      Search.Pending.Add(ParentNode);
   }

   constexpr bool bAllowShrinking = false;

   while (Search.Pending.Num() > 0)
   {
      UOSEGenericGraphNode* ThisNode = Search.PopPending();
      Search.Visited.Add(ThisNode);

      // Fire the callback for the current node
      const bool ContinueIterating = Callback(ThisNode);
      if (!ContinueIterating)
      {
         return;
      }

      // Find the next level of parent nodes to iterate over
      for (UOSEGenericGraphNode* ParentNode : ThisNode->ParentNodes)
      {
         if (!Search.Visited.Contains(ParentNode))
         {
            Search.PushPending(ParentNode);
         }
      }
   }
}

struct UOSEGenericGraph::FCycleCheckerState
{
   TArray<UOSEGenericGraphNode*> Stack;
   TSet<UOSEGenericGraphNode*> Pending;
   TSet<UOSEGenericGraphNode*> Completed;
   int32 NumCyclesDetected = 0;
};

int32 UOSEGenericGraph::FindCycles(FCycleDetectedCallback CycleDetectedCallback) const
{
   FCycleCheckerState State;

   for (UOSEGenericGraphNode* Node : RootNodes)
   {
      State.Stack.Reset();
      FindCyclesInternal(Node, CycleDetectedCallback, State);
   }

   for (UOSEGenericGraphNode* Node : AllNodes)
   {
      if (!State.Completed.Contains(Node))
      {
         State.Stack.Reset();
         FindCyclesInternal(Node, CycleDetectedCallback, State);
      }
   }

   return State.NumCyclesDetected;
}

// static
void UOSEGenericGraph::FindCyclesInternal(UOSEGenericGraphNode* Node, FCycleDetectedCallback CycleDetectedCallback, FCycleCheckerState& State)
{
   check(Node != nullptr);
   if (State.Completed.Contains(Node))
   {
      return;
   }

   // basic algorithm:
   // Does a depth-first search of the graph, keeping track of which nodes
   // are currently being visited and which ones have had all their children visited.
   // If it finds a node that is still being visited, then that must be a cycle.
   State.Pending.Add(Node);
   State.Stack.Push(Node);

   for (UOSEGenericGraphNode* ChildNode : Node->ChildrenNodes)
   {
      if (State.Pending.Contains(ChildNode))
      {
         // A cycle
         State.Stack.Push(ChildNode);

         // slice array to only include cycle
         const int32 FirstIndex = State.Stack.IndexOfByKey(ChildNode);
         check(State.Stack.IsValidIndex(FirstIndex));
         TArrayView<UOSEGenericGraphNode*> Slice = TArrayView<UOSEGenericGraphNode*>(State.Stack).Slice(FirstIndex, State.Stack.Num() - FirstIndex);
         CycleDetectedCallback(Slice);
         ++State.NumCyclesDetected;
         State.Stack.Pop();
      }
      else if (!State.Completed.Contains(ChildNode))
      {
         FindCyclesInternal(ChildNode, CycleDetectedCallback, State);
      }
   }
   State.Pending.Remove(Node);
   State.Completed.Add(Node);
   State.Stack.Pop();
}

FString UOSEGenericGraph::ToDebugString() const
{
   TStringBuilder<512> Result;

   int Level = 0;
   TArray<UOSEGenericGraphNode*> CurrLevelNodes = RootNodes;
   TArray<UOSEGenericGraphNode*> NextLevelNodes;

   while (CurrLevelNodes.Num() != 0)
   {
      for (int i = 0; i < CurrLevelNodes.Num(); ++i)
      {
         UOSEGenericGraphNode* Node = CurrLevelNodes[i];
         check(Node != nullptr);

         Result.Appendf(TEXT("%s, Level %d\n"), *Node->GetNodeDebugName(), Level);

         for (int j = 0; j < Node->ChildrenNodes.Num(); ++j)
         {
            NextLevelNodes.Add(Node->ChildrenNodes[j]);
         }
      }

      CurrLevelNodes = NextLevelNodes;
      NextLevelNodes.Reset();
      ++Level;
   }

   return Result.ToString();
}

void UOSEGenericGraph::ClearGraph()
{
   for (int i = 0; i < AllNodes.Num(); ++i)
   {
      UOSEGenericGraphNode* Node = AllNodes[i];
      if (Node)
      {
         Node->ParentNodes.Empty();
         Node->ChildrenNodes.Empty();
         Node->Edges.Empty();
      }
   }

   AllNodes.Empty();
   RootNodes.Empty();
}

UOSEGenericGraphNode* UOSEGenericGraph::GetNodeByGuid(const FGuid& Guid) const
{
   if (NodeIdMap.Num() == 0 && AllNodes.Num() > 0)
   {
      const_cast<UOSEGenericGraph*>(this)->RebuildNodeIdMap();
   }
   UOSEGenericGraphNode* const* NodePtr = NodeIdMap.Find(Guid);
   return (NodePtr != nullptr) ? *NodePtr : nullptr;
}

void UOSEGenericGraph::RebuildNodeIdMap()
{
   NodeIdMap.Reset();
   for (UOSEGenericGraphNode* Node : AllNodes)
   {
      NodeIdMap.Add(Node->NodeId, Node);
   }
}

#if WITH_EDITOR
FText UOSEGenericGraph::GetGraphTypeDisplayName() const
{
   // Checks if the string is in the form "MODClassName".
   // Assumes this is true if the string has length >= 5, the first four chars are uppercase, and the fifth char is lowercase.
   auto TryGetModulePrefix = [](FStringView Name, FStringView& OutModuleName, FStringView& OutClassName) -> bool
   {
      if (Name.Len() >= 5)
      {
         for (int32 Idx = 0; Idx < 4; Idx++)
         {
            if (!FChar::IsUpper(Name[Idx]))
            {
               return false;
            }
         }
         if (FChar::IsLower(Name[4]))
         {
            OutModuleName = Name.SubStr(0, 3);
            OutClassName = Name.SubStr(3, INT32_MAX);
            return true;
         }
      }
      return false;
   };

   constexpr bool IsBool = false;
   const FString DisplayString = FName::NameToDisplayString(GetClass()->GetFName().ToString(), IsBool);

   FStringView ModuleName;
   FStringView ClassName;
   if (TryGetModulePrefix(DisplayString, ModuleName, ClassName))
   {
      return FText::Format(INVTEXT("[{0}] {1}"), FText::FromStringView(ModuleName), FText::FromStringView(ClassName));
   }

   return FText::FromString(DisplayString);
}

bool UOSEGenericGraph::_CheckForDuplicateNodes() const
{
   for (int i = 0; i < AllNodes.Num() - 1; ++i)
   {
      for (int j = i + 1; j < AllNodes.Num(); ++j)
      {
         if (AllNodes[j] == AllNodes[i])
         {
            return true;
         }
      }
   }
   return false;
}
#endif

#undef LOCTEXT_NAMESPACE
