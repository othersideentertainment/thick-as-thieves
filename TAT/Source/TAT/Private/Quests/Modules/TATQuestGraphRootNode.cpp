// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Quests/Modules/TATQuestGraphRootNode.h"

// tat
#include "Quests/Modules/TATQuestGraph.h"
#include "Quests/Modules/TATQuestGraphUtil.h"

// ue
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestGraphRootNode)


UTATQuestGraphRootNode::UTATQuestGraphRootNode()
{
#if WITH_EDITORONLY_DATA
   ContextMenuName = FText::FromString(TEXT("Root"));
   ParentLimitType = EOSEGenericGraphNodeLimit::Limited;
   ParentLimit = 0;
#endif
}

bool UTATQuestGraphRootNode::ExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const
{
   if (!Super::ExecuteNode(params, ctx))
   {
      return false;
   }
   if (!Enabled)
   {
      TAT_QUESTGRAPH_LOG_ERROR(ctx, this, TEXT("Executing disabled root node!"));
   }
   return Enabled;
}

FString UTATQuestGraphRootNode::GetNodeDebugName() const
{
   return FString::Printf(TEXT("RootNode(%s)"), Enabled ? TEXT("") : TEXT("DISABLED"));
}

#if WITH_EDITOR

EDataValidationResult UTATQuestGraphRootNode::IsDataValid(FDataValidationContext& context) const
{
   int32 numIssues = 0;
   if (ParentNodes.Num() > 0)
   {
      ++numIssues;
      context.AddError(FText::FromString(FString::Format(TEXT("{0}: Root node has {1} parent nodes. Root nodes are not allowed to have parent nodes"), {
         GetNodeDebugName(), ParentNodes.Num() })));
   }
   return (numIssues == 0) ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}

TSharedPtr<SWidget> UTATQuestGraphRootNode::ConstructNodeBodyWidget()
{
   return SNew(SBorder)
      .Padding(FMargin(4.0f))
      .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
      .BorderBackgroundColor(TATQuestGraphUtil::MakeAttribute(this, +[](UTATQuestGraphRootNode* self) -> FSlateColor
      {
         return TATQuestGraphUtil::kGenericColor.CopyWithNewOpacity(self->Enabled ? 0.5f : 0.2f);
      }))
      [
         SNew(SBorder)
         .Padding(FMargin(6.0f))
         .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
         .BorderBackgroundColor(FSlateColor(FLinearColor(0, 0, 0, 0.75f)))
         [
            SNew(STextBlock)
            .Font(FAppStyle::GetFontStyle("BoldFont"))
            .Text(TATQuestGraphUtil::MakeAttribute(this, +[](UTATQuestGraphRootNode* self) -> FText
            {
               if (UTATQuestGraph* questGraph = Cast<UTATQuestGraph>(self->GetGraph()))
               {
                  static const FText questGraphText = FText::FromString(TEXT("Quest Graph"));
                  if (questGraph->Map.IsNull())
                  {
                     return questGraphText;
                  }
                  return FText::FormatOrdered(INVTEXT("{0} for {1}"), questGraphText, FText::FromString(questGraph->Map.GetAssetName()));
               }
               return INVTEXT("Module");
            }))
         ]
      ];
}

FLinearColor UTATQuestGraphRootNode::GetBackgroundColor() const
{
   FLinearColor result = TATQuestGraphUtil::kGenericColor;
   if (!Enabled)
   {
      result.A = 0.05f;
   }
   return result;
}

bool UTATQuestGraphRootNode::CanCreateConnectionFrom(UOSEGenericGraphNode* other, int32 numberOfParentNodes, FText& errorMessage)
{
   errorMessage = FText::FromString(TEXT("Root nodes must be starting nodes"));
   return false;
}

#endif // WITH_EDITOR

