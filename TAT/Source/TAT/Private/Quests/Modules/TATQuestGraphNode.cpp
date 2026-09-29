// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Quests/Modules/TATQuestGraphNode.h"

// tat
#include "Math/TATMath.h"
#include "Quests/Modules/TATQuestGraph.h"
#include "Quests/Modules/TATQuestGraphEdge.h"
#include "Quests/Modules/TATQuestGraphRandomHelpers.h"
#include "Quests/Modules/TATQuestGraphStatsSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestGraphNode)


UTATQuestGraphNode::UTATQuestGraphNode()
{
#if WITH_EDITORONLY_DATA
   CompatibleGraphType = UTATQuestGraphBase::StaticClass();
#endif
}

UTATQuestGraphNode* UTATQuestGraphNode::SelectNextNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const
{
   using FNodeChoice = TTATRandomWeight<UTATQuestGraphNode*>;
   TArray<FNodeChoice, TInlineAllocator<16>> choices;
   int highestPrioritySeen = FTATQuestGraphCompatibility::IncompatiblePriority;

   const bool checkForceSelectNodes = params.ForceSelectNodes.Num() > 0;
   bool checkCompatibility = ChildrenNodes.Num() > 1;

   for (UOSEGenericGraphNode* child : ChildrenNodes)
   {
      UTATQuestGraphNode* node = Cast<UTATQuestGraphNode>(child);
      UTATQuestGraphEdge* edge = Cast<UTATQuestGraphEdge>(GetEdge(child));
      if (node == nullptr || edge == nullptr)
      {
         continue;
      }

      if (checkForceSelectNodes && params.ForceSelectNodes.Contains(node->AsHandle()))
      {
         return node;
      }

      if ((!edge->QuestTagQuery.IsEmpty() && !edge->QuestTagQuery.Matches(ctx.QuestTags))
         || (!edge->WorldTagQuery.IsEmpty() && !edge->WorldTagQuery.Matches(params.WorldTags)))
      {
         continue;
      }

      if (edge->AlwaysTakePathIfConditionsMatch)
      {
         return node;
      }

      if (checkCompatibility)
      {
         const FTATQuestGraphCompatibility compatibility = ctx.LookupNodeCompatibility(node, params);
         const int priority = compatibility.GetPriority();
         if (priority < highestPrioritySeen)
         {
            // NOTE: Incompatible paths are lower priority than everything else, but are not unconditionally
            //       skipped if they are the only options available. This prevents it from aborting if all
            //       options are incompatible, since that seems fragile as a fallback. There may be situations
            //       where stopping is desired, but we would have to no longer treat that as an error. It is
            //       just harder to infer intent from the current structure.
            continue;
         }
         else if (priority > highestPrioritySeen)
         {
            // if priority is greater than max priority, any previous choices must be lower
            highestPrioritySeen = priority;
            choices.Reset();
         }
      }

      choices.Add({ node, edge->ChanceToTakePath });
   }

   FTATXoshiroRandomStream randomStream = TATQuestGraphRandomHelpers::CreateStreamForPath(params.MapSeed, this);
   const FNodeChoice* result = TATMath::SelectRandomItemWeighted(
      TConstArrayView<FNodeChoice>(choices),
      [](FNodeChoice item) -> float
      {
         return item.Weight;
      },
      [&randomStream](float minVal, float maxVal) -> float
      {
         return randomStream.NextFloatInRange(minVal, maxVal);
      });

   if (result != nullptr)
   {
      UTATQuestGraphNode* resultNode = result->Value;
      check(resultNode != nullptr);
      return resultNode;
   }

   return nullptr;
}

FTATQuestGraphCompatibility UTATQuestGraphNode::GetSelfCompatibility(const FTATQuestGraphEvalParams& params, const FTATQuestGraphEvalContext& ctx, FTATQuestGraphCompatibilityCache& cache) const
{
    return FTATQuestGraphCompatibility();
}

FTATQuestGraphSceneCache::MaskType UTATQuestGraphNode::GetSceneMask(FTATQuestGraphSceneCache& cache) const
{
   return 0;
}

#if WITH_EDITOR
bool UTATQuestGraphNode::GetNodeIndicatorVisible() const
{
   return GEngine->GetEngineSubsystem<UTATQuestGraphStatsSubsystem>()->GetLastSimulatedSpawnRate(this).IsSet();
}

FText UTATQuestGraphNode::GetNodeIndicatorText() const
{
   if (TOptional<float> chance = GEngine->GetEngineSubsystem<UTATQuestGraphStatsSubsystem>()->GetLastSimulatedSpawnRate(this))
   {
      TStringBuilder<32> builder;
      builder.Appendf(TEXT("%.1f%%"), *chance);
      return FText::FromStringView(MakeStringView(builder));
   }
   return FText::GetEmpty();
}
#endif
