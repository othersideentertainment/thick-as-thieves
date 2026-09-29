// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Quests/Modules/TATQuestGraphTypes.h"

// tat
#include "Quests/Modules/TATQuestGraphNode.h"
#include "Quests/Modules/TATQuestGraphObjectiveNode.h"
#include "Quests/Modules/TATQuestGraphSubgraphNode.h"
#include "Variation/SceneVariants/TATSceneVariantConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestGraphTypes)

DEFINE_LOG_CATEGORY(LogTATQuestGraph);

FString FTATQuestGraphLogMessage::ToString() const
{
   return FString::Printf(TEXT("[%s] %s: %s"),
      *((SourceNode != nullptr) ? SourceNode->GetNodeDebugName() : TEXT("NULL")),
      (IsError ? TEXT("Error") : TEXT("Info")),
      *Message);
}

const UTATQuestGraphObjectiveNode* FTATQuestGraphResult::TryGetObjectiveNode() const
{
   return ObjectiveNodeHandle.GetNode<UTATQuestGraphObjectiveNode>();
}

void FTATQuestGraphEvalContext::AddSceneVariant(TObjectPtr<UTATSceneVariantConfig> variant)
{
   if (variant)
   {
      _sceneVariants.Add(variant);
      if (const UTATSceneAsset* scene = variant->GetParentScene())
      {
         // not sure if interesting to be unique
         _usedScenes.AddUnique(scene);
         _usedSceneMask |= _compatibilityCache.GetMaskForScene(scene);
         _compatibilityCache.SetUsedSceneMask(_usedSceneMask);
      }
   }
}

void FTATQuestGraphEvalContext::AddSceneVariants(TConstArrayView<TObjectPtr<UTATSceneVariantConfig>> variants)
{
   _sceneVariants.Reserve(variants.Num());
   _usedScenes.Reserve(variants.Num());
   for (TObjectPtr<UTATSceneVariantConfig> variant : variants)
   {
      AddSceneVariant(variant);
   }
}

FTATQuestGraphCompatibility FTATQuestGraphEvalContext::GetCompatibilityForVariant(const UTATSceneVariantConfig* variant) const
{
   if (variant == nullptr)
   {
      return FTATQuestGraphCompatibility{};
   }

   if (int variantIndex = _sceneVariants.IndexOfByKey(variant); variantIndex != INDEX_NONE)
   {
      return FTATQuestGraphCompatibility{ .ReachableVariantsMask = 1ull << variantIndex };
   }

   return FTATQuestGraphCompatibility{ .Incompatible = _usedScenes.Contains(variant->GetParentScene()) };
}

FTATQuestGraphCompatibility FTATQuestGraphEvalContext::LookupNodeCompatibility(const UTATQuestGraphNode* node, const FTATQuestGraphEvalParams& params)
{
   return _compatibilityCache.Lookup(node, params, *this);
}

void FTATQuestGraphEvalContext::ForEachModule(TFunctionRef<void(UTATQuestGraphModule*)> callback) const
{
   for (const UTATQuestGraphNode* node : QuestPlan)
   {
      const UTATQuestGraphSubgraphNode* subgraphNode = Cast<UTATQuestGraphSubgraphNode>(node);
      if (subgraphNode != nullptr && subgraphNode->Module != nullptr)
      {
         callback(subgraphNode->Module);
      }
   }
}


FTATQuestGraphSceneCache::MaskType FTATQuestGraphSceneCache::GetReachableMaskForNode(const UTATQuestGraphNode* node)
{
   check(node);

   if (const MaskType* found = _cache.Find(node))
   {
      return *found;
   }

   MaskType mask = node->GetSceneMask(*this);
   for (UOSEGenericGraphNode* child : node->ChildrenNodes)
   {
      if (UTATQuestGraphNode* childNode = Cast<UTATQuestGraphNode>(child))
      {
         mask |= GetReachableMaskForNode(childNode);
      }
   }

   _cache.Add(node, mask);
   return mask;
}

void FTATQuestGraphCompatibility::FPathCombiner::Add(const FTATQuestGraphCompatibility& path)
{
   _variantsMask |= path.ReachableVariantsMask;
   _currentCount += 1;
   if (path.Incompatible)
   {
      _incompatibleCount += 1;
   }
}

FTATQuestGraphCompatibility FTATQuestGraphCompatibility::FPathCombiner::Build() const
{
   const bool incompatible = _incompatibleCount > 0 && _incompatibleCount == _currentCount;
   return FTATQuestGraphCompatibility{ .ReachableVariantsMask = _variantsMask, .Incompatible = incompatible };
}

FTATQuestGraphCompatibility FTATQuestGraphCompatibilityCache::Lookup(const UTATQuestGraphNode* node, const FTATQuestGraphEvalParams& params, const FTATQuestGraphEvalContext& context)
{
   check(node);

   if (const FEntry* found = _cache.Find(node);
       found && (found->UnaccountedSceneMask & _usedSceneMask) == 0)
   {
      // Cached entry is still valid if no scenes were added that are reachable from this node (but were not used when calculated)
      return found->Compatibility;
   }

   const SceneMask reachableSceneMask = GetReachableMaskForNode(node);
   FTATQuestGraphCompatibility compat;
   SceneMask unhandledSceneMask = 0;

   // only calculate if the used scenes overlap with the used scenes
   if (reachableSceneMask & _usedSceneMask)
   {
      auto findChildCompat = [&]() {
         FTATQuestGraphCompatibility::FPathCombiner combiner;
         for (UOSEGenericGraphNode* child : node->ChildrenNodes)
         {
            if (UTATQuestGraphNode* childNode = Cast<UTATQuestGraphNode>(child))
            {
               combiner.Add(Lookup(childNode, params, context));
            }
         }
         return combiner.Build();
         };

      compat = node->GetSelfCompatibility(params, context, *this);
      compat = compat.Concat(findChildCompat());
      // unhandled scene mask has scenes that are reachable from this node, but not used yet
      unhandledSceneMask = reachableSceneMask & ~_usedSceneMask;
   }

   _cache.Add(node, FEntry{ compat, unhandledSceneMask });
   return compat;
}

void FTATQuestGraphCompatibilityCache::Reset()
{
   _cache.Reset();
   // CONSIDER: allow keeping the scene cache intact in simulation, since it does not depend on the run
   _sceneCache = FTATQuestGraphSceneCache();
   _usedSceneMask = 0;
}
