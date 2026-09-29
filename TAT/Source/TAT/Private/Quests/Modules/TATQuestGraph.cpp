// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Quests/Modules/TATQuestGraph.h"

// tat
#include "Quests/Modules/TATQuestGraphEdge.h"
#include "Quests/Modules/TATQuestGraphRootNode.h"
#include "Quests/Modules/TATQuestGraphCondenseNode.h"
#include "Quests/Modules/TATQuestGraphLockCombinationNode.h"
#include "Quests/Modules/TATQuestGraphSubgraphNode.h"
#include "Quests/Modules/TATQuestGraphSceneVariantNode.h"
#include "Quests/Modules/TATQuestGraphTagNode.h"
#include "Quests/Modules/TATQuestGraphObjectiveNode.h"
#include "Quests/Modules/TATQuestGraphPropertyNode.h"
#include "Quests/Modules/TATQuestGraphTextReplacementNode.h"
#include "Quests/Modules/TATQuestGraphClueSetNode.h"
#include "Quests/Modules/TATQuestGraphMapCheckContext.h"

// ue
#include "Misc/DataValidation.h"
#if WITH_EDITOR
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#endif

#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/AssetRegistryTagsContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestGraph)


UTATQuestGraphBase::UTATQuestGraphBase()
{
   NodeTypes = {
      UTATQuestGraphRootNode::StaticClass(),
      UTATQuestGraphCondenseNode::StaticClass(),
      UTATQuestGraphSceneVariantNode::StaticClass(),
      UTATQuestGraphTagNode::StaticClass(),
      UTATQuestGraphClueSetNode::StaticClass(),
      UTATQuestGraphClueSetForSpawnNode::StaticClass(),
      UTATQuestGraphObjectiveNode::StaticClass(),
      UTATQuestGraphPropertyNode::StaticClass(),
      UTATQuestGraphLockCombinationNode::StaticClass(),
      UTATQuestGraphTextReplacementNode::StaticClass(),
      UTATQuestGraphTextReplacementSourceNode::StaticClass(),
   };
   EdgeType = UTATQuestGraphEdge::StaticClass();
   bEdgeTransitionEnabled = true;

#if WITH_EDITORONLY_DATA
   bCanRenameNode = true;
   bCanBeCyclical = false;
#endif
}

UTATQuestGraphRootNode* UTATQuestGraphBase::SelectRootNode(int32* outNumRootNodesFound) const
{
   int32 numEnabledRootNodes = 0;
   UTATQuestGraphRootNode* firstEnabledRootNode = nullptr;

   for (UOSEGenericGraphNode* n : RootNodes)
   {
      UTATQuestGraphRootNode* rootNode = Cast<UTATQuestGraphRootNode>(n);
      if (rootNode != nullptr && rootNode->Enabled)
      {
         if (firstEnabledRootNode == nullptr)
         {
            firstEnabledRootNode = rootNode;
         }
         ++numEnabledRootNodes;
      }
   }

   if (numEnabledRootNodes > 1)
   {
      UE_LOG(LogTATQuestGraph, Error, TEXT("Graph %s has %i enabled root nodes (expected exactly one!). Using the first one."), *GetName(), numEnabledRootNodes);
   }

   if (outNumRootNodesFound != nullptr)
   {
      *outNumRootNodesFound = numEnabledRootNodes;
   }

   return firstEnabledRootNode;
}

bool UTATQuestGraphBase::EvalQuestGraph(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx, UTATQuestGraphRootNode* rootNode) const
{
   TRACE_CPUPROFILER_EVENT_SCOPE(UTATQuestGraphBase::EvalQuestGraph)
   if (rootNode == nullptr)
   {
      rootNode = SelectRootNode();
   }
   if (rootNode == nullptr)
   {
      UE_LOG(LogTATQuestGraph, Error, TEXT("Failed to evaluate quest graph %s: no valid root node found"), *GetName());
      return false;
   }

   UTATQuestGraphNode* currentNode = rootNode;
   while (currentNode != nullptr)
   {
      // Execute this node and add it to the quest plan
      if (currentNode->ExecuteNode(params, ctx))
      {
         ctx.QuestPlan.Add(currentNode);
      }
      else
      {
         TAT_QUESTGRAPH_LOG_ERROR(ctx, nullptr, TEXT("Failed to eval quest graph during node %s"), *currentNode->GetName());
         return false;
      }

      // If the node wants to do anything after execution, do that now.
      // For example, execute a subgraph, which should happen _after_ adding the subgraph node to the quest plan.
      if (!currentNode->PostExecuteNode(params, ctx))
      {
         TAT_QUESTGRAPH_LOG_ERROR(ctx, nullptr, TEXT("Failed to eval quest graph just after node %s"), *currentNode->GetName());
         return false;
      }

      if (currentNode->ChildrenNodes.Num() > 0)
      {
         UTATQuestGraphNode* prevNode = currentNode;
         currentNode = currentNode->SelectNextNode(params, ctx);
         if (currentNode == nullptr)
         {
            TAT_QUESTGRAPH_LOG_ERROR(ctx, nullptr, TEXT("Failed to select next node (from %s)"), *prevNode->GetName());
            return false;
         }
      }
      else
      {
         currentNode = nullptr;
      }
   }

   return true;
}

#if WITH_EDITOR
EDataValidationResult UTATQuestGraphBase::IsDataValid(FDataValidationContext& context) const
{
   const EDataValidationResult baseResult = Super::IsDataValid(context);

   int32 numRootNodesFound = 0;
   UTATQuestGraphRootNode* startingRootNode = SelectRootNode(&numRootNodesFound);
   if (startingRootNode == nullptr)
   {
      context.AddError(FText::FromString(TEXT("Quest graph does not have a valid root node.")));
   }
   if (numRootNodesFound > 1)
   {
      context.AddError(FText::FromString(FString::Format(TEXT("Found {0} possible root nodes. Quest graphs must have exactly one root node."), { numRootNodesFound })));
   }

   EDataValidationResult traverseResult = EDataValidationResult::NotValidated;

   const UTATQuestGraphNode* firstNodeThatWantsMap = nullptr;

   if (startingRootNode != nullptr)
   {
      TraverseNodes(EOSEGenericGraphSearchMode::DepthFirstSearch, [&](UOSEGenericGraphNode* node) -> bool
      {
         if (!ensure(node != nullptr))
         {
            return UOSEGenericGraph::TraverseContinue;
         }

         const UTATQuestGraphNode* questGraphNode = Cast<UTATQuestGraphNode>(node);
         if (questGraphNode == nullptr)
         {
            context.AddError(FText::FromString(FString::Printf(TEXT("%s: Node is not a child class of UTATQuestGraphNode"), *node->GetNodeDebugName())));
            return UOSEGenericGraph::TraverseContinue;
         }

         // TODO: include enough context to know which node has the error
         const EDataValidationResult result = CombineDataValidationResults(IsNodeDataValid(context, questGraphNode), questGraphNode->IsDataValid(context));
         if (result == EDataValidationResult::Invalid)
         {
            traverseResult = result;
         }

         if (firstNodeThatWantsMap == nullptr && questGraphNode->RequireMapForValidation())
         {
            firstNodeThatWantsMap = questGraphNode;
         }

         return UOSEGenericGraph::TraverseContinue;
      }, startingRootNode);
   }

   // Just checking Map in the base class rather than Module, since can avoid multiple
   // traversals of the graph
   if (Map.IsNull() && firstNodeThatWantsMap != nullptr)
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("QuestGraph does not specify a Map, but has nodes that require it, such as %s"), *firstNodeThatWantsMap->GetNodeDebugName())));
   }

   if (context.GetValidationUsecase() != EDataValidationUsecase::Commandlet)
   {
      // If the map happens to already be loaded, check against that
      // This seems to work for level instances in PIE
      if (const UWorld* loadedMap = Map.Get())
      {
         FTATQuestGraphMapCheckContext checkContext = FTATQuestGraphMapCheckContext::Create(loadedMap,
            [&context] (TSharedRef<FTokenizedMessage> message) { context.AddMessage(MoveTemp(message)); });
         ValidateAgainstWorld(checkContext);
      }
   }

   const bool isValid = traverseResult != EDataValidationResult::Invalid && context.GetNumWarnings() + context.GetNumErrors() == 0;
   return CombineDataValidationResults(baseResult, isValid ? EDataValidationResult::Valid : EDataValidationResult::Invalid);
}
EDataValidationResult UTATQuestGraphBase::IsNodeDataValid(FDataValidationContext& context, const UTATQuestGraphNode* node) const
{
   check(node != nullptr);
   return EDataValidationResult::NotValidated;
}

void UTATQuestGraphBase::ValidateAgainstWorld(FTATQuestGraphMapCheckContext& context) const
{
   if(!context.TryVisit(this))
   {
      return;
   }
   
   // Check only reachable nodes.
   // Slightly more expensive, but means that unreachable nodes are effectively "commented out" for validation purposes
   if (UTATQuestGraphRootNode* startingRootNode = SelectRootNode())
   {
      TraverseNodes(EOSEGenericGraphSearchMode::DepthFirstSearch, [&context](const UOSEGenericGraphNode* node) -> bool
      {
         if (const UTATQuestGraphNode* questGraphNode = Cast<UTATQuestGraphNode>(node))
         {
            questGraphNode->ValidateAgainstWorld(context);
         }
         return UOSEGenericGraph::TraverseContinue;
      }, startingRootNode);
   }
}
#endif // WITH_EDITOR

UTATQuestGraphModule::UTATQuestGraphModule()
{
}

#if WITH_EDITOR
EDataValidationResult UTATQuestGraphModule::IsDataValid(FDataValidationContext& context) const
{
   return Super::IsDataValid(context);
}
#endif // WITH_EDITOR

UTATQuestGraph::UTATQuestGraph()
{
   NodeTypes.Add(UTATQuestGraphSubgraphNode::StaticClass());
}

bool UTATQuestGraph::DebugEvalQuestGraph(const UObject* contextObject, int32 seed, const FGameplayTagContainer& questTags, const FGameplayTagContainer& worldTags,
   FTATQuestGraphEvalContext& outQuestGraphContext) const
{
   FTATQuestGraphEvalParams params{};
   params.World = GEngine->GetWorldFromContextObject(contextObject, EGetWorldErrorMode::Assert);
   params.WorldTags = worldTags;
   params.MapSeed = seed;

   outQuestGraphContext = FTATQuestGraphEvalContext{};
   outQuestGraphContext.QuestTags = questTags;

   return EvalQuestGraph(params, outQuestGraphContext);
}

#if WITH_EDITOR
void UTATQuestGraph::GetAssetRegistryTags(FAssetRegistryTagsContext context) const
{
   Super::GetAssetRegistryTags(context);

   // Don't bother putting in the cook for now
   if (!context.IsCooking() && !Map.IsNull())
   {
      context.AddTag(FAssetRegistryTag(NAME_Map, Map.GetLongPackageName(), FAssetRegistryTag::TT_Alphabetical));
   }
}

TArray<TSoftObjectPtr<UTATQuestGraph>> UTATQuestGraph::FindForMap(const TSoftObjectPtr<UWorld>& map)
{
   TRACE_CPUPROFILER_EVENT_SCOPE(UTATMatchQuestDescription::FindForMap)
   const IAssetRegistry& assetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();

   FARFilter filter;
   filter.ClassPaths.Add(UTATQuestGraph::StaticClass()->GetClassPathName());
   filter.bIncludeOnlyOnDiskAssets = true;
   filter.TagsAndValues.Add(NAME_Map, map.GetLongPackageName());

   TArray<TSoftObjectPtr<UTATQuestGraph>> result;
   assetRegistry.EnumerateAssets(filter, [&result](const FAssetData& assetData)
      {
         result.Emplace(assetData.GetSoftObjectPath());
         return true;
      });

   return result;
}

EDataValidationResult UTATQuestGraph::IsDataValid(FDataValidationContext& context) const
{
   Super::IsDataValid(context);

   if(Map.IsNull())
   {
      context.AddWarning(INVTEXT("Map is null (Required for main quest graphs)"));
   }

   return (context.GetNumErrors() + context.GetNumWarnings()) ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
