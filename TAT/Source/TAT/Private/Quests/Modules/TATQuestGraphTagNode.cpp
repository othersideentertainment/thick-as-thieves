// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Quests/Modules/TATQuestGraphTagNode.h"

// tat
#include "Quests/Modules/TATQuestGraphRandomHelpers.h"

// ue
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestGraphTagNode)


UTATQuestGraphTagNode::UTATQuestGraphTagNode()
{
#if WITH_EDITORONLY_DATA
   ContextMenuName = FText::FromString(TEXT("Selector: Quest Tag"));
   SelectorTypeDisplayName = TEXT("Quest Tag");
#endif
}

bool UTATQuestGraphTagNode::ExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const
{
   if (!Super::ExecuteNode(params, ctx))
   {
      return false;
   }
   FTATXoshiroRandomStream randomStream  = TATQuestGraphRandomHelpers::CreateStreamForNode(params.MapSeed, this);
   TATQuestGraphRandomHelpers::EvalRandomWeightedMulti<FTATQuestRandomWeight>(randomStream, SelectCount, TagWeights,
      [&](const FTATQuestRandomWeight& item)
      {
         if (item.Tag.IsValid())
         {
            ctx.QuestTags.AddTag(item.Tag);
            TAT_QUESTGRAPH_LOG_MESSAGE(ctx, this, TEXT("[TAG SELECTOR] Added quest tag %s"), *item.Tag.ToString());
         }
         else
         {
            TAT_QUESTGRAPH_LOG_ERROR(ctx, this, TEXT("[TAG SELECTOR] Tag select node %s picked an invalid tag!"), *GetNodeDebugName());
         }
      });
   return true;
}

#if WITH_EDITOR
EDataValidationResult UTATQuestGraphTagNode::IsDataValid(FDataValidationContext& context) const
{
   const EDataValidationResult baseResult = Super::IsDataValid(context);
   int32 numIssues = 0;

   if (TagWeights.Num() == 0)
   {
      ++numIssues;
      context.AddError(FText::FromString(FString::Printf(TEXT("%s: Empty tag weights array"), *GetNodeDebugName())));
   }

   float totalWeight = 0.0f;
   for (int32 i = 0; i < TagWeights.Num(); i++)
   {
      totalWeight += TagWeights[i].Weight;

      if (!TagWeights[i].Tag.IsValid())
      {
         ++numIssues;
         context.AddError(FText::FromString(FString::Printf(TEXT("%s: Invalid tag at index %i"), *GetNodeDebugName(), i)));
      }
   }

   if (TagWeights.Num() > 1 && totalWeight <= 0)
   {
      ++numIssues;
      context.AddError(FText::FromString(FString::Printf(TEXT("%s: Total weight is %.2f (total weight must be greater than zero)"), *GetNodeDebugName(), totalWeight)));
   }

   return CombineDataValidationResults(baseResult, (numIssues == 0) ? EDataValidationResult::Valid : EDataValidationResult::Invalid);
}
#endif
