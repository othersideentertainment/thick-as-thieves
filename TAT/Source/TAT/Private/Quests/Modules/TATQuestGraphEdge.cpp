// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Quests/Modules/TATQuestGraphEdge.h"

// tat
#include "Quests/Modules/TATQuestGraphUtil.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestGraphEdge)


UTATQuestGraphEdge::UTATQuestGraphEdge()
{
}

#if WITH_EDITOR

FLinearColor UTATQuestGraphEdge::GetEdgeColor() const
{
   if (AlwaysTakePathIfConditionsMatch)
   {
      return TATQuestGraphUtil::kEdgeConditionMandatoryColor;
   }
   if (!QuestTagQuery.IsEmpty() || !WorldTagQuery.IsEmpty())
   {
      return TATQuestGraphUtil::kEdgeConditionColor;
   }
   return Super::GetEdgeColor();
}

const FSlateBrush* UTATQuestGraphEdge::GetEdgeIcon() const
{
   // When the edge is always taken and has no tag queries, it has no text on it.
   // In that case, show the default icon so it's not completely empty.
   if (!ShowQuery && AlwaysTakePathIfConditionsMatch && QuestTagQuery.IsEmpty() && WorldTagQuery.IsEmpty())
   {
      return Super::GetEdgeIcon();
   }
   return nullptr;
}

FText UTATQuestGraphEdge::GetEdgeTooltipText() const
{
   constexpr bool detailed = true;
   return _GetEdgeDescription(detailed);
}

FText UTATQuestGraphEdge::GetEdgeDisplayTitle() const
{
   return _GetEdgeDescription(ShowQuery);
}

#endif // #if WITH_EDITOR

FText UTATQuestGraphEdge::_GetEdgeDescription(bool detailed) const
{
   TArray<FText> parts;

   if (!AlwaysTakePathIfConditionsMatch)
   {
      FNumberFormattingOptions opts{};
      opts.MinimumFractionalDigits = 0;
      opts.MaximumFractionalDigits = 2;
      const FTextFormat weightFmtString = detailed ? INVTEXT("Weight: {0}") : INVTEXT("{0}");
      parts.Add(FText::FormatOrdered(weightFmtString, FText::AsNumber(ChanceToTakePath, &opts)));
   }

   if (detailed)
   {
      if (!QuestTagQuery.IsEmpty())
      {
         parts.Add(FText::FormatOrdered(INVTEXT("Quest: {0}"), FText::FromString(QuestTagQuery.GetDescription())));
      }
      if (!WorldTagQuery.IsEmpty())
      {
         parts.Add(FText::FormatOrdered(INVTEXT("World: {0}"), FText::FromString(WorldTagQuery.GetDescription())));
      }
   }

   return FText::Join(INVTEXT("\n"), parts);
}
