// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Quests/Modules/TATQuestGraphSceneVariantNode.h"

// tat
#include "Quests/Modules/TATQuestGraphRandomHelpers.h"

// ue
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestGraphSceneVariantNode)


UTATQuestGraphSceneVariantNode::UTATQuestGraphSceneVariantNode()
{
#if WITH_EDITORONLY_DATA
   ContextMenuName = FText::FromString(TEXT("Selector: Scene Variant"));
#endif
}

bool UTATQuestGraphSceneVariantNode::ExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const
{
   if (!Super::ExecuteNode(params, ctx))
   {
      return false;
   }

   TArray<int32, TInlineAllocator<32>> candidateIndices;
   int32 numSelected = 0;

   // filter valid candidates, and count ones already selected
   for (int i = 0; i < SceneVariants.Num(); ++i)
   {
      UTATSceneVariantConfig* variant = SceneVariants[i].SceneVariant;
      if (variant == nullptr)
      {
         continue;
      }

      if (ctx.GetSceneVariants().Contains(variant))
      {
         numSelected++;
         TAT_QUESTGRAPH_LOG_MESSAGE(ctx, this, TEXT("[SCENE VARIANT] Scene variant %s already selected, counting"), *GetNameSafe(variant));
      }
      else if (!ctx.GetUsedScenes().Contains(variant->GetParentScene()))
      {
         candidateIndices.Add(i);
      }
   }

   FTATXoshiroRandomStream randomStream = TATQuestGraphRandomHelpers::CreateStreamForNode(params.MapSeed, this);
   const int32 unclampedCount = FMath::Max(0, (SelectCount.Min == SelectCount.Max) ? SelectCount.Min : randomStream.NextInt32InRange(SelectCount.Min, SelectCount.Max));
   const int32 clampedCount = FMath::Min(unclampedCount, candidateIndices.Num() + numSelected);

   // Select remaining variants
   while (numSelected < clampedCount && candidateIndices.Num() > 0)
   {
      const int32* found = TATMath::SelectRandomItemWeighted<int>(
         candidateIndices,
         [this](int itemIndex) { return SceneVariants[itemIndex].Weight; },
         [&randomStream](float minVal, float maxVal) -> float { return randomStream.NextFloatInRange(minVal, maxVal); });
      if (!ensure(found))
      {
         break;
      }

      UTATSceneVariantConfig* variant = SceneVariants[*found].SceneVariant;
      check(variant);
      TAT_QUESTGRAPH_LOG_MESSAGE(ctx, this, TEXT("[SCENE VARIANT] Added scene variant %s"), *GetNameSafe(variant));
      ctx.AddSceneVariant(variant);
      numSelected++;

      if (numSelected == clampedCount)
      {
         break;
      }

      // would be slightly more convenient (obviously correct) if it returned the index rather than the pointer
      const int32 candidateIndexIndex = found - candidateIndices.GetData();
      check(candidateIndices.IsValidIndex(candidateIndexIndex));
      candidateIndices.RemoveAtSwap(candidateIndexIndex);

      // remove candidates that have that have the scene, since they aren't compatible
      candidateIndices.RemoveAll([this, parent = variant->GetParentScene()](int index) {
         return SceneVariants[index].SceneVariant->GetParentScene() == parent;
         });
   }

   if (numSelected < unclampedCount)
   {
      TAT_QUESTGRAPH_LOG_ERROR(ctx, this, TEXT("[SCENE VARIANT] Wanted to select %d variants, but only selected %d"), unclampedCount, numSelected);
   }

   return true;
}

FTATQuestGraphCompatibility UTATQuestGraphSceneVariantNode::GetSelfCompatibility(const FTATQuestGraphEvalParams& params, const FTATQuestGraphEvalContext& ctx, FTATQuestGraphCompatibilityCache& cache) const
{
   FTATQuestGraphCompatibility::FPathCombiner combiner;
   for (const FTATQuestRandomWeight_SceneVariant& entry : SceneVariants)
   {
      if (entry.SceneVariant)
      {
         combiner.Add(ctx.GetCompatibilityForVariant(entry.SceneVariant));
      }
   }

   return combiner.Build();
}

FTATQuestGraphSceneCache::MaskType UTATQuestGraphSceneVariantNode::GetSceneMask(FTATQuestGraphSceneCache& cache) const
{
   FTATQuestGraphSceneCache::MaskType mask = 0;
   for (const FTATQuestRandomWeight_SceneVariant& entry : SceneVariants)
   {
      const UTATSceneVariantConfig* variant = entry.SceneVariant;
      const UTATSceneAsset* scene = variant ? variant->GetParentScene() : nullptr;

      if (scene)
      {
         mask |= cache.GetMaskForScene(scene);
      }
   }
   return mask;
}

#if WITH_EDITOR
EDataValidationResult UTATQuestGraphSceneVariantNode::IsDataValid(FDataValidationContext& context) const
{
   const EDataValidationResult baseResult = Super::IsDataValid(context);
   int32 numIssues = 0;

   if (SceneVariants.Num() == 0)
   {
      ++numIssues;
      context.AddError(FText::FromString(FString::Printf(TEXT("%s: Empty scene variants array"), *GetNodeDebugName())));
   }

   float totalWeight = 0.0f;
   for (int32 i = 0; i < SceneVariants.Num(); i++)
   {
      totalWeight += SceneVariants[i].Weight;

      if (!SceneVariants[i].SceneVariant)
      {
         ++numIssues;
         context.AddError(FText::FromString(FString::Printf(TEXT("%s: Invalid scene variant at index %i"), *GetNodeDebugName(), i)));
      }
   }

   if (SceneVariants.Num() > 1 && totalWeight <= 0)
   {
      ++numIssues;
      context.AddError(FText::FromString(FString::Printf(TEXT("%s: Total weight is %.2f (total weight must be greater than zero)"), *GetNodeDebugName(), totalWeight)));
   }

   return CombineDataValidationResults(baseResult, (numIssues == 0) ? EDataValidationResult::Valid : EDataValidationResult::Invalid);
}
#endif
